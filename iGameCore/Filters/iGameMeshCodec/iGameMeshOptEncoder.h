#ifndef MeshOptEncoder_h
#define MeshOptEncoder_h

#include "iGameMacro.h"

#include "iGameFlatArray.h"
#include "iGameThreadPool.h"
#include "iGameProgressObserver.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshEncoderAdapter.h"
#include "iGameMeshOptCodec.h"

#include "iGameMeshCodecLZMA.h"
#include <functional>

IGAME_NAMESPACE_BEGIN
class MeshOptEncoder : public MeshOptCodec {
public:
    MeshOptEncoder(std::ofstream& bytestreamFile, DataObject::Pointer dataObj, MeshOptParameters& params,
                   ParamInformation& inputParams, bool isDebugMode)
        : // TODO: 临时添加，和m_DataObj定义在一起
          MeshOptCodec(params), m_ParamInformation(inputParams), m_BytestreamFile(bytestreamFile), m_DataObj(dataObj),
          m_EncoderAdapter(new MeshEncoderAdapter(dataObj)), m_IsDebugMode(isDebugMode) {
        this->kVertexScoreTableStrip = {
                {0.f, 1.000f, 1.000f, 1.000f, 0.453f, 0.561f, 0.490f, 0.459f, 0.179f, 0.526f, 0.000f, 0.227f, 0.184f,
                 0.490f, 0.112f, 0.050f, 0.131f},
                {0.f, 0.956f, 0.786f, 0.577f, 0.558f, 0.618f, 0.549f, 0.499f, 0.489f},
        };
    };

    bool Execute() {
        // TODO: 结构化网格不需要输出topo 仅依靠点数据就可以构建mesh

        // 暂时不考虑多帧和multiblock的问题
        // 原则 函数使用void* float*时 必须在调用函数之前自行开辟空间
        // 由于这类指针很有可能不直接用array托管 所以函数无义务在其内开辟空间
        // 如果函数参数为vector 则一般由函数自行管理空间大小
        this->ParamsInit();

        std::vector<unsigned int> pointIdRemap;
        std::vector<unsigned int> cellIdsRemap;

        PayloadBuffer geomPayload(PayloadType::kGeometryBrick);
        this->GeomEncoder(geomPayload, pointIdRemap);
        UpdateProgress(0.2);

        PayloadBuffer topoPayload(PayloadType::kTopologyBrick);
        this->TopoEncoder(topoPayload, cellIdsRemap, pointIdRemap);
        UpdateProgress(0.4);

        PayloadBuffer attrPayload(PayloadType::kAttributeBrick);
        this->AttrEncoder(attrPayload, cellIdsRemap, pointIdRemap);
        UpdateProgress(0.6);

        PayloadBuffer paramPayload(PayloadType::kParameterSet);
        this->ParamsEncoder(paramPayload);
        UpdateProgress(0.8);

        // lzma
        int compressLevel = 10;
        int numThreads = 12;

        PayloadBuffer geomCompressed(PayloadType::kGeometryBrick);
        PayloadBuffer topoCompressed(PayloadType::kTopologyBrick);
        PayloadBuffer attrCompressed(PayloadType::kAttributeBrick);
        PayloadBuffer paramCompressed(PayloadType::kParameterSet);

        MeshCodecLZMA::Compress(geomCompressed, geomPayload, compressLevel, numThreads);
        MeshCodecLZMA::Compress(topoCompressed, topoPayload, compressLevel, numThreads);
        MeshCodecLZMA::Compress(attrCompressed, attrPayload, compressLevel, numThreads);
        MeshCodecLZMA::Compress(paramCompressed, paramPayload, compressLevel, numThreads);

        //WriteBuf(paramPayload, this->m_BytestreamFile); // 必须先写参数信息
        //WriteBuf(geomPayload, this->m_BytestreamFile);
        //WriteBuf(topoPayload, this->m_BytestreamFile);
        //WriteBuf(attrPayload, this->m_BytestreamFile);

        WriteBuf(paramCompressed, this->m_BytestreamFile); // 必须先写参数信息
        WriteBuf(geomCompressed, this->m_BytestreamFile);
        WriteBuf(topoCompressed, this->m_BytestreamFile);
        WriteBuf(attrCompressed, this->m_BytestreamFile);

        UpdateProgress(1.0);
        //std::vector<char> input(geomPayload.begin(), geomPayload.end());
        //std::vector<char> result;
        //MeshCodecLZMA::Compress(result, input, 10, 12);

        //PayloadBuffer output;

        //int ret = MeshCodecLZMA::Decompress(output, result);

        return true;
    }

    void TopoEncoder(PayloadBuffer& payload, std::vector<unsigned int>& cellRemap,
                     const std::vector<unsigned int> pointIdRemap) {
        IGsize pointCount = this->m_EncoderAdapter->GetNumberOfPoints();
        IGsize cellCount = this->m_EncoderAdapter->GetNumberOfCells();
        IGsize fixedCellSize = this->m_EncoderAdapter->GetFixedCellSize();
        IGsize cellBufferSize = this->m_EncoderAdapter->GetCellIdBufferSize();
        IGsize cellOffsetSize = this->m_EncoderAdapter->GetCellIdOffsetSize();
        bool isFixedCellSize = this->m_EncoderAdapter->IsFixedCellSize();

        UnsignedIntArray::Pointer cellIdBuffer = this->m_EncoderAdapter->GetCellIdBuffer();

        // 应用顶点重映射
        UnsignedIntArray::Pointer remappedCellIdBuffer = UnsignedIntArray::New();
        remappedCellIdBuffer->Resize(cellBufferSize);

        for (size_t i = 0; i < cellBufferSize; ++i) {
            unsigned int index = cellIdBuffer->GetValue(i);
            remappedCellIdBuffer->SetValue(i, pointIdRemap[index]);
        }

        // 重排序以及写入
        std::vector<unsigned int> optCellBuffer(cellBufferSize);
        std::vector<unsigned int> optCellOffset(cellOffsetSize);
        cellRemap.resize(cellCount);
        if (isFixedCellSize) {
            this->OptimizeCellVertexCache(optCellBuffer.data(), cellRemap.data(), remappedCellIdBuffer->RawPointer(),
                                          cellBufferSize, pointCount, fixedCellSize);
        } else {
            UnsignedIntArray::Pointer cellOffset = this->m_EncoderAdapter->GetCellIdOffset();
            this->OptimizeHybirdCellVertexCache(optCellBuffer.data(), optCellOffset.data(), cellRemap.data(),
                                                remappedCellIdBuffer->RawPointer(), cellOffset->RawPointer(),
                                                cellBufferSize, cellOffsetSize, pointCount, cellCount);
        }

        // 调用fastpfor 写入payload
        // 稍微开大一些空间用于存储编码结果 但是一般编码结果是小于这个长度的
        //std::vector<uint32_t> encodeFastPForBuffer;

        // 分开写入 记录count 这里使用unsigned char是一个历史遗留问题 暂时不解决
        // 顺序分别是 optCellBuffer optCellOffset remappedCellTypes
        std::vector<unsigned char> outputTopo;

        // 写入offset
        std::vector<unsigned char> encodeBuffer;

        encodeBuffer.clear();
        this->CellBufferEncoder(encodeBuffer, optCellBuffer);
        outputTopo.insert(outputTopo.end(), encodeBuffer.begin(), encodeBuffer.end());
        this->m_Params.topoParams.cellBufferBinaryCount = encodeBuffer.size();

        //encodeFastPForBuffer.resize(cellBufferSize + 1024);
        //this->CellBufferEncoder(encodeFastPForBuffer, optCellBuffer);

        this->m_Params.topoParams.cellSizeBinaryCount = 0;
        if (!isFixedCellSize) {
            // 写入size 非结构化网格也有可能是定长size
            // 所以这里直接写就行了 如果是定长size 这里就会被自动跳过
            encodeBuffer.clear();
            this->CellSizeEncoder(encodeBuffer, optCellOffset); // 在解码的时候处理完rle就结束了
            outputTopo.insert(outputTopo.end(), encodeBuffer.begin(), encodeBuffer.end());
            this->m_Params.topoParams.cellSizeBinaryCount = encodeBuffer.size();
        }

        // 如果非结构化 还要写入type
        this->m_Params.topoParams.cellTypeBinaryCount = 0;
        if (this->m_Params.meshType == IG_UNSTRUCTURED_MESH) {
            UnsignedIntArray::Pointer cellTypes = this->m_EncoderAdapter->GetCellTypes();
            // 应用map做对cell类型做重排序
            std::vector<uint32_t> remappedCellTypes(cellCount);
            for (int i = 0; i < cellCount; i++) { remappedCellTypes[cellRemap[i]] = cellTypes->GetValue(i); }

            encodeBuffer.clear();
            this->CellTypeEncoder(encodeBuffer, remappedCellTypes);
            outputTopo.insert(outputTopo.end(), encodeBuffer.begin(), encodeBuffer.end());
            this->m_Params.topoParams.cellTypeBinaryCount = encodeBuffer.size();
        }

        //int baseSize = cellBufferSize + cellOffsetSize + 1024;
        ////encodeFastPForBuffer.resize(baseSize);
        //// optCellBuffer + optCellOffset
        //optCellBuffer.insert(optCellBuffer.end(),
        //    optCellOffset.begin(), optCellOffset.end());

        //// 额 非结构化应该都是不定长offset吧...?
        //// 也有可能定长的 哈哈
        //if (this->m_Params.meshType == IG_UNSTRUCTURED_MESH)
        //{
        //    //encodeFastPForBuffer.resize(baseSize + cellCount);
        //    UnsignedIntArray::Pointer cellTypes = this->m_EncoderAdapter->GetCellTypes();
        //    // 应用map做对cell类型做重排序
        //    std::vector<uint32_t> remappedCellTypes(cellCount);
        //    for (int i = 0; i < cellCount; i++)
        //    {
        //        remappedCellTypes[cellRemap[i]] = cellTypes->GetValue(i);
        //    }

        //    // optCellBuffer + optCellOffset + remappedCellTypes
        //    optCellBuffer.insert(optCellBuffer.end(),
        //        remappedCellTypes.begin(), remappedCellTypes.end());
        //}
        //// TODO offset换用字典编码
        //this->CellBufferEncoder(outputTopo, optCellBuffer);

        //payload.resize(encodeFastPForBuffer.size() * sizeof(uint32_t));
        //std::memcpy(payload.data(),
        //    encodeFastPForBuffer.data(),
        //    encodeFastPForBuffer.size() * sizeof(uint32_t)
        //);

        payload.resize(outputTopo.size());
        std::memcpy(payload.data(), outputTopo.data(), outputTopo.size());
    }
    
private:
    std::ofstream& m_BytestreamFile;
    DataObject::Pointer m_DataObj;
    MeshEncoderAdapter* m_EncoderAdapter;
    ParamInformation m_ParamInformation;
    bool m_IsDebugMode;

    ProgressObserver* m_Progress{nullptr};

    void UpdateProgress(double p) { 
        if (m_Progress) { 
            m_Progress->UpdateProgress(p);
        } else {
            m_Progress = ProgressObserver::Instance();
        }
    }


    void ParamsInit() {
        // 判断网格类型
        this->m_Params.meshType = this->m_EncoderAdapter->GetMeshType();
        // multiblock网格类型暂不支持
        assert(this->m_Params.meshType == IG_SURFACE_MESH || this->m_Params.meshType == IG_VOLUME_MESH ||
               this->m_Params.meshType == IG_STRUCTURED_MESH || this->m_Params.meshType == IG_UNSTRUCTURED_MESH);
        if (this->m_Params.meshType == IG_STRUCTURED_MESH) {
            std::memcpy(&this->m_Params.structuredMeshParams.axisSize,
                        DynamicCast<StructuredMesh>(this->m_DataObj)->GetDimensionSize(), 3 * sizeof(int));
        }

        this->m_Params.topoParams.fixedCellSize = this->m_EncoderAdapter->GetFixedCellSize();
        this->m_Params.topoParams.cellBufferSize = this->m_EncoderAdapter->GetCellIdBufferSize();
        this->m_Params.topoParams.cellCount = this->m_EncoderAdapter->GetNumberOfCells();

        // 以后改成GUI调控
        this->m_Params.geomParams.valueSize = sizeof(float); // float
        this->m_Params.geomParams.elementCount = this->m_EncoderAdapter->GetNumberOfPoints();
        this->m_Params.geomParams.dimension = 3;

        this->m_Params.geomParams.scale = -1;
        //this->m_Params.geomParams.quantMode = QuantMode::Float;
        //this->m_Params.geomParams.quantParam = 16;
        this->m_Params.geomParams.quantMode = m_ParamInformation.PointQuantMode;
        this->m_Params.geomParams.quantParam = m_ParamInformation.PointQuantizedBits;
        assert(this->m_Params.geomParams.quantParam >= 0 && this->m_Params.geomParams.quantParam <= 23);

        // 属性参数
        auto allAttrs = this->m_DataObj->GetAttributeSet()->GetAllAttributes();
        for (int attrIndex = 0; attrIndex < allAttrs->GetNumberOfElements(); attrIndex++) {
            AttributeSet::Attribute attr = allAttrs->GetElement(attrIndex);
            MeshOptAttrParameters attrParams;

            std::memset(attrParams.name, '\0', sizeof(attrParams.name));
            attr.pointer->GetName().copy(attrParams.name, sizeof(attrParams.name) - 1); // 保留一位给\0

            attrParams.dimension = attr.pointer->GetDimension();
            attrParams.type = attr.type;
            attrParams.attachmentType = attr.attachmentType;

            attrParams.valueSize = attr.pointer->GetArrayTypedSize();
            attrParams.elementCount = attr.pointer->GetNumberOfElements();

            attrParams.scale = -1;
            //attrParams.quantMode = QuantMode::Float;
            //attrParams.quantParam = 16;
            attrParams.quantMode = m_ParamInformation.AttrbQuantMode;
            attrParams.quantParam = m_ParamInformation.AttrbQuantizedBits;

            assert(attrParams.quantMode >= 0 && attrParams.quantMode <= 23);

            this->m_Params.attrParams.push_back(attrParams);
        }
        this->m_Params.attrCount = this->m_Params.attrParams.size();
    }

    void ParamsEncoder(PayloadBuffer& payload) {
        MeshOptParametersWithoutAttr paramsWoAttr = static_cast<MeshOptParametersWithoutAttr>(this->m_Params);
        IGsize staticSize = sizeof(MeshOptParametersWithoutAttr);
        IGsize dynamicSize = this->m_Params.attrParams.size() * sizeof(MeshOptAttrParameters);

        payload.resize(staticSize + dynamicSize);

        // 写入静态数据
        std::memcpy(payload.data(), &paramsWoAttr, staticSize);

        // 写入动态数据
        if (!this->m_Params.attrParams.empty()) {
            std::memcpy(payload.data() + staticSize, this->m_Params.attrParams.data(), dynamicSize);
        }
    }

    void FloatEncoder(std::vector<unsigned char>& dest,
                      const void* source, // float* double*
                      const MeshOptFloatParameters& floatParams, std::string debugFloatName = "") {
        IGsize elementCount = floatParams.elementCount;
        int dimension = floatParams.dimension;
        IGsize valueCount = elementCount * dimension;

        // void* -> float* / double*
        //std::vector<float> convertFloatBuffer;
        //const float* floatSource = nullptr;

        //std::vector<double> convertDoubleBuffer;
        //const double* doubleSource = nullptr;

        std::vector<float> floatBuffer;
        const float* floatSource = nullptr;

        std::vector<double> doubleBuffer;
        const double* doubleSource = nullptr;

        if (floatParams.valueSize == sizeof(float)) {
            floatSource = static_cast<const float*>(source);
        } else if (floatParams.valueSize == sizeof(double)) {
            doubleSource = static_cast<const double*>(source);
        }

        // 这个flag不一定反映原始的数据 而有可能随计算过程发生变化
        // 但应保证的是 floatFlag && doubleFlag == false
        bool floatFlag = (floatSource != nullptr);
        bool doubleFlag = (doubleSource != nullptr);

        // void* -> vector<float> / vector<double>
        // 定点数化
        if (floatParams.scale != -1) {
            // 这里分开写近乎一样的代码 是避免在多线程中反复考虑数据类型
            if (floatFlag) {
                floatBuffer.resize(valueCount);
                ThreadPool::parallelFor(0, elementCount, [&](int start, int end) -> void {
                    for (int i = start; i < end; i++) {
                        floatBuffer[i] = static_cast<int32_t>(floatSource[i] * floatParams.scale) / floatParams.scale;
                    }
                });
            } else {
                doubleBuffer.resize(valueCount);
                ThreadPool::parallelFor(0, elementCount, [&](int start, int end) -> void {
                    for (int i = start; i < end; i++) {
                        doubleBuffer[i] = static_cast<int32_t>(doubleSource[i] * floatParams.scale) / floatParams.scale;
                    }
                });
            }
        } else {
            if (floatFlag) {
                floatBuffer.resize(valueCount);
                std::memcpy(floatBuffer.data(), floatSource, valueCount * sizeof(float));
            } else {
                doubleBuffer.resize(valueCount);
                std::memcpy(doubleBuffer.data(), doubleSource, valueCount * sizeof(double));
            }
        }

        // 量化
        int encodeVertexSize;
        int encodeElementCount;
        switch (floatParams.quantMode) {
            case QuantMode::None: {
                encodeVertexSize = floatParams.valueSize * dimension;
                encodeElementCount = elementCount;
                break;
            }
            case QuantMode::FP16: {
                int quantBufferSize = (size_t) ceil(valueCount / 2.);

                encodeVertexSize = sizeof(float);
                encodeElementCount = quantBufferSize;

                // 量化后 一个float占有半字节 也就是16bit
                // 所以需要先另外找一片区域存放 然后合起来拼接到floatBuffer

                std::vector<uint32_t> quantiBuffer(quantBufferSize);
                const size_t blockSize = 65536;
                assert(blockSize % 2 == 0);

                ThreadPool* tp = ThreadPool::Instance();
                size_t tpCount = (size_t) ceil(valueCount / (double) blockSize);
                std::vector<std::future<void>> tpResult(tpCount);
                // 1 elementCount 和 valueCount 没有考虑清楚
                // 2 threads没有wait

                if (floatFlag) {
                    for (int i = 0, tpCursor = 0; i < valueCount; i += blockSize, tpCursor++) {
                        size_t length = std::min(valueCount - i, (IGsize)blockSize);
                        tpResult[tpCursor] = tp->Commit(
                                [&](int start, int end) -> void {
                                    for (int j = start; j < end; j += 2) {
                                        uint32_t a = (uint32_t) meshopt_quantizeHalf(floatBuffer[j]);
                                        uint32_t b =
                                                (uint32_t) (end - j == 1 ? 0
                                                                         : meshopt_quantizeHalf(floatBuffer[j + 1]));
                                        int shift = sizeof(unsigned short) * 8;
                                        uint32_t group = (a << shift) | b;
                                        std::memcpy(&quantiBuffer[(j + 1) / 2], &group, sizeof(uint32_t));
                                    }
                                },
                                i, i + length);
                    }
                } else {
                    for (int i = 0, tpCursor = 0; i < valueCount; i += blockSize, tpCursor++) {
                        size_t length = std::min(valueCount - i, (IGsize)blockSize);
                        tpResult[tpCursor] = tp->Commit(
                                [&](int start, int end) -> void {
                                    for (int j = start; j < end; j += 2) {
                                        uint32_t a = (uint32_t) meshopt_quantizeHalf((float) doubleBuffer[j]);
                                        uint32_t b = (uint32_t) (end - j == 1 ? 0
                                                                              : meshopt_quantizeHalf(
                                                                                        (float) doubleBuffer[j + 1]));
                                        int shift = sizeof(unsigned short) * 8;
                                        uint32_t group = (a << shift) | b;
                                        std::memcpy(&quantiBuffer[(j + 1) / 2], &group, sizeof(uint32_t));
                                    }
                                },
                                i, i + length);
                    }
                }

                for (int i = 0; i < tpCount; i++) { tpResult[i].wait(); }

                floatBuffer.clear();
                floatBuffer.resize(quantBufferSize);
                std::memcpy(floatBuffer.data(), quantiBuffer.data(), quantBufferSize * sizeof(float));

                floatFlag = true;
                doubleFlag = false;

                break;
            }
            case QuantMode::
                    Float: // 这种量化没有对应的反量化步骤 因为这种量化实际上是下调了float的精度 而不是把他转为整数
            {
                encodeVertexSize = sizeof(float) * dimension;
                encodeElementCount = elementCount;
                if (floatFlag) {
                    ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
                        for (int i = start; i < end; i++) {
                            floatBuffer[i] = meshopt_quantizeFloat(floatBuffer[i], floatParams.quantParam);
                        }
                    });
                } else {
                    ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
                        for (int i = start; i < end; i++) {
                            floatBuffer[i] = meshopt_quantizeFloat(doubleBuffer[i], floatParams.quantParam);
                        }
                    });
                }

                // 这步操作会强行降低精度到float 而且在数值精度上也会下降
                floatFlag = true;
                doubleFlag = false;

                break;
            }
            default:
                break;
        }

        // 编码
        assert(floatFlag && doubleFlag == false);
        dest.resize(meshopt_encodeVertexBufferBound(encodeElementCount, encodeVertexSize));

        if (floatFlag) {
            dest.resize(meshopt_encodeVertexBuffer(dest.data(), dest.size(), floatBuffer.data(), encodeElementCount,
                                                   encodeVertexSize));
        } else {
            dest.resize(meshopt_encodeVertexBuffer(dest.data(), dest.size(), doubleBuffer.data(), encodeElementCount,
                                                   encodeVertexSize));
        }

        if (m_IsDebugMode) {
            float err = CalError(source, dest, floatParams);
            std::cout << "float data name: " << debugFloatName << " MAPE: " << err << std::endl;
        }

        return;

        /*if (floatParams.scale != -1)
    {
        if (floatSource)
        {
            convertFloatBuffer.resize(valueCount);
            std::transform(
                floatSource,
                floatSource + valueCount,
                convertFloatBuffer.begin(),
                [&](float x) {return static_cast<int32_t>(x * floatParams.scale) / floatParams.scale; }
            );
        }
        else if (doubleSource)
        {
            convertDoubleBuffer.resize(valueCount);
            std::transform(
                doubleSource,
                doubleSource + valueCount,
                convertFloatBuffer.begin(),
                [&](float x) {return static_cast<int32_t>(x * floatParams.scale) / floatParams.scale; }
            );
        }
    }
    else
    {
        if (floatSource)
        {
            convertFloatBuffer.resize(valueCount);
            std::transform(
                floatSource,
                floatSource + valueCount,
                convertFloatBuffer.begin(),
                [&](float x) {return static_cast<double>(x); }
            );
        }
        else if (doubleSource)
        {
            convertDoubleBuffer.resize(valueCount);
            convertDoubleBuffer.assign(
                doubleSource,
                doubleSource + valueCount
            );
        }
    }*/

        //// 量化和编码
        //switch (floatParams.quantMode)
        //{
        //case QuantMode::None:
        //{
        //    IGsize valueSize = floatParams.valueSize;
        //    dest.resize(meshopt_encodeVertexBufferBound(
        //        elementCount, valueSize * dimension));

        //    if (floatParams.valueSize == sizeof(float))
        //    {
        //        dest.resize(meshopt_encodeVertexBuffer(
        //            dest.data(),
        //            dest.size(),
        //            floatBuffer.data(),
        //            elementCount, valueSize * dimension
        //        ));
        //    }
        //    else if (floatParams.valueSize == sizeof(double))
        //    {
        //        dest.resize(meshopt_encodeVertexBuffer(
        //            dest.data(),
        //            dest.size(),
        //            doubleBuffer.data(),
        //            elementCount, valueSize * dimension
        //        ));
        //    }
        //    break;
        //}
        //case QuantMode::FP16:
        //{
        //    IGsize valueSize = sizeof(unsigned short);
        //    std::vector<unsigned short> fp16Buffer(valueCount);

        //    // 这里修成两个unsigned short拼一块 transform肯定是用不了了
        //    // 先普通代码换多线程测一下速度 多线程也可以用来拼这个
        //    if (floatParams.valueSize == sizeof(float))
        //    {
        //        std::transform(
        //            fp16Buffer.begin(),
        //            fp16Buffer.end(),
        //            floatBuffer.begin(),
        //            [&](double x) {return meshopt_quantizeHalf(x); }
        //        );
        //    }
        //    else if (floatParams.valueSize == sizeof(double))
        //    {
        //        std::transform(
        //            fp16Buffer.begin(),
        //            fp16Buffer.end(),
        //            doubleBuffer.begin(),
        //            [&](double x) {return meshopt_quantizeHalf(x); }
        //        );
        //    }
        //
        //    dest.resize(meshopt_encodeVertexBuffer(
        //        dest.data(),
        //        dest.size(),
        //        fp16Buffer.data(),
        //        elementCount, valueSize * dimension
        //    ));

        //    break;
        //}
        //case QuantMode::Float: // 这种量化没有对应的反量化步骤 因为这种量化实际上是下调了float的精度 而不是把他转为整数
        //{
        //    IGsize valueSize = sizeof(float);

        //    if (floatParams.valueSize == sizeof(float))
        //    {
        //        std::transform(
        //            floatBuffer.begin(),
        //            floatBuffer.end(),
        //            floatBuffer.begin(),
        //            [&](double x) {return meshopt_quantizeFloat(x, floatParams.quantParam); }
        //        );
        //    }
        //    else if (floatParams.valueSize == sizeof(double))
        //    {
        //        std::transform(
        //            doubleBuffer.begin(),
        //            doubleBuffer.end(),
        //            floatBuffer.begin(),
        //            [&](double x) {return meshopt_quantizeFloat(x, floatParams.quantParam); }
        //        );
        //    }

        //    dest.resize(meshopt_encodeVertexBuffer(
        //        dest.data(),
        //        dest.size(),
        //        floatBuffer.data(),
        //        elementCount, valueSize * dimension
        //    ));

        //    break;
        //}
        //default:
        //    break;
        //}

        // 解码测试
        //std::vector<float> out(valueCount);
        //int resvb = meshopt_decodeVertexBuffer(out.data(),
        //    elementCount, sizeof(Vector3f), &dest[0], dest.size());

        //return;
    }

    float CalError(const void* source, const std::vector<unsigned char>& encoded,
                   const MeshOptFloatParameters& floatParams) {
        IGsize valueCount = floatParams.elementCount * floatParams.dimension;

        std::vector<float> sourceFloat(valueCount);
        std::vector<float> encodedFloat(valueCount);

        if (floatParams.valueSize == sizeof(float)) {
            ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
                for (int i = start; i < end; i++) { sourceFloat[i] = static_cast<const float*>(source)[i]; }
            });
        } else if (floatParams.valueSize == sizeof(double)) {
            ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
                for (int i = start; i < end; i++) { sourceFloat[i] = static_cast<const double*>(source)[i]; }
            });
        }


        // decode
        std::vector<float> encodedFloatBuffer;
        std::vector<double> encodedDoubleBuffer;

        bool encodedFloatFlag = false;
        bool encodedDoubleFlag = false;


        switch (floatParams.quantMode) {
            case QuantMode::None: {
                IGsize vertexSize = floatParams.valueSize * floatParams.dimension;
                if (floatParams.valueSize == sizeof(float)) {
                    encodedFloatBuffer.resize(valueCount);
                    encodedFloatFlag = true;

                    meshopt_decodeVertexBuffer(encodedFloatBuffer.data(), floatParams.elementCount, vertexSize,
                                               encoded.data(), encoded.size());
                } else {
                    encodedDoubleBuffer.resize(valueCount);
                    encodedDoubleFlag = true;

                    meshopt_decodeVertexBuffer(encodedDoubleBuffer.data(), floatParams.elementCount, vertexSize,
                                               encoded.data(), encoded.size());
                }

                break;
            }
            case QuantMode::FP16: {
                // 这种情况的输出只有float
                encodedFloatBuffer.resize(valueCount); // 开多了没问题 别开少了就行
                encodedFloatFlag = true;

                // 由于量化是两个float并一个存 导致不一定输出还是原始dimension的倍数
                // 所以只好分离的编码 而不能成组编码
                IGsize vertexSize = sizeof(float);
                size_t quantValueCount = (size_t) ceil(valueCount / 2.);

                // 先把量化后的序列读到floatBuffer
                meshopt_decodeVertexBuffer(encodedFloatBuffer.data(), quantValueCount, vertexSize, encoded.data(),
                                           encoded.size());

                // 反量化
                std::vector<float> dequantBuffer(valueCount);

                const size_t blockSize = 65536;
                ThreadPool::Pointer tp = ThreadPool::Instance();
                int tpCount = (size_t) ceil(quantValueCount / (double) blockSize);
                std::vector<std::future<void>> tpResult(tpCount);

                for (int i = 0, tpCursor = 0; i < quantValueCount;
                     i += blockSize, tpCursor++) // 这里quantiValueCount是quantize后的序列的元素数量
                {
                    size_t length = std::min(quantValueCount - i, blockSize);
                    tpResult[tpCursor] = tp->Commit(
                            [&](int start, int end) -> void {
                                for (int j = start; j < end; j++) {
                                    // 读取一个值
                                    uint32_t group;
                                    std::memcpy(&group, &encodedFloatBuffer[j], sizeof(uint32_t));
                                    int shift = sizeof(unsigned short) * 8;

                                    float a = meshopt_dequantizeHalf((unsigned short) (group >> shift));
                                    float b = meshopt_dequantizeHalf((unsigned short) (group & ((1 << shift) - 1)));

                                    dequantBuffer[j * 2] = a;
                                    if (j * 2 + 1 != valueCount) { dequantBuffer[j * 2 + 1] = b; }
                                }
                            },
                            i, i + length);
                }

                for (int i = 0; i < tpCount; i++) { tpResult[i].wait(); }

                // 转移结果
                encodedFloatBuffer.clear();
                encodedFloatBuffer.resize(valueCount);
                std::memcpy(encodedFloatBuffer.data(), dequantBuffer.data(), valueCount * sizeof(float));

                break;
            }
            case QuantMode::Float: {
                // 这种情况的输出只有float
                IGsize vertexSize = sizeof(float) * floatParams.dimension;
                encodedFloatFlag = true;

                encodedFloatBuffer.resize(valueCount);
                meshopt_decodeVertexBuffer(encodedFloatBuffer.data(), floatParams.elementCount, vertexSize,
                                           encoded.data(), encoded.size());

                break;
            }
            default:
                break;
        }


        // 写入dest
        // 两种普通情形 实际产生的数据类型和dest对应
        if (encodedFloatFlag) {
            encodedFloat = encodedFloatBuffer;
        } else if (encodedDoubleFlag) {
            encodedFloat.assign(encodedDoubleBuffer.begin(), encodedDoubleBuffer.end());
        }

        return CalMAPE(sourceFloat.data(), encodedFloat.data(), floatParams);
    }

    float CalMAPE(const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) {
        float globalError = 0;
        IGsize valueCount = floatParams.elementCount * floatParams.dimension;

        for (int i = 0; i < valueCount; i++) { globalError += std::abs(source[i] - encoded[i]) / std::abs(source[i]); }

        return globalError / valueCount;
    }

    void GeomEncoder(PayloadBuffer& payload, std::vector<unsigned int>& pointIdRemap) {
        Points::Pointer points = this->m_EncoderAdapter->GetPoints();
        IGsize pointCount = this->m_Params.geomParams.elementCount;
        IGsize pointBufferSize = pointCount * this->m_Params.geomParams.dimension; // 一个顶点3个维度

        // 重映射
        pointIdRemap.resize(pointCount);
        meshopt_spatialSortRemap(pointIdRemap.data(), points->RawPointer(), pointCount, sizeof(Vector3f));

        // 部署重映射
        std::vector<float> remappedPointBuffer(pointBufferSize);
        meshopt_remapVertexBuffer(remappedPointBuffer.data(), points->RawPointer(), pointCount, sizeof(Vector3f),
                                  pointIdRemap.data());

        std::vector<unsigned char> encodedFloat;
        this->FloatEncoder(encodedFloat, remappedPointBuffer.data(), this->m_Params.geomParams, "geom");

        payload.resize(encodedFloat.size());
        std::memcpy(payload.data(), encodedFloat.data(), encodedFloat.size());
    }

    void AttrEncoder(PayloadBuffer& payload, const std::vector<unsigned int>& cellRemap,
                     const std::vector<unsigned int>& pointRemap) {
        ElementArray<AttributeSet::Attribute>::Pointer attrs = this->m_DataObj->GetAttributeSet()->GetAllAttributes();
        std::vector<std::vector<unsigned char>> outFloats(this->m_Params.attrParams.size());
        size_t threadNum = std::max(this->m_Params.attrParams.size() / 2, (size_t) 1);

        ThreadPool::parallelFor(
                0, this->m_Params.attrParams.size(),
                [&](int start, int end) -> void {
                    for (int i = start; i < end; i++) {
                        auto& attr = attrs->GetElement(i);
                        auto& params = this->m_Params.attrParams[i];
                        int dimension = params.dimension;
                        int elementCount = params.elementCount;
                        int valueCount = params.elementCount * params.dimension;

                        // 部署remap
                        std::vector<float> remappedFloatAttrBuffer;
                        std::vector<double> remappedDoubleAttrBuffer;
                        if (params.valueSize == sizeof(float)) {
                            remappedFloatAttrBuffer.resize(valueCount);
                            ThreadPool::parallelFor(0, elementCount, [&](int start, int end) -> void {
                                for (int j = start; j < end; j++) {
                                    igIndex remapIndex =
                                            (params.attachmentType == IG_POINT ? pointRemap[j] : cellRemap[j]);
                                    std::vector<double> values;
                                    attr.pointer->GetElement(j, values);

                                    for (int k = 0; k < params.dimension; k++) {
                                        remappedFloatAttrBuffer[remapIndex * params.dimension + k] = values[k];
                                    }
                                }
                            });
                        } else {
                            remappedDoubleAttrBuffer.resize(valueCount);
                            ThreadPool::parallelFor(0, elementCount, [&](int start, int end) -> void {
                                for (int j = start; j < end; j++) {
                                    igIndex remapIndex =
                                            (params.attachmentType == IG_POINT ? pointRemap[j] : cellRemap[j]);
                                    std::vector<double> values;
                                    attr.pointer->GetElement(j, values);

                                    for (int k = 0; k < params.dimension; k++) {
                                        remappedDoubleAttrBuffer[remapIndex * params.dimension + k] = values[k];
                                    }
                                }
                            });
                        }

                        // 编码
                        std::vector<unsigned char> encodedFloat;
                        if (params.valueSize == sizeof(float)) {
                            this->FloatEncoder(encodedFloat, remappedFloatAttrBuffer.data(), params, params.name);
                        } else if (params.valueSize == sizeof(double)) {
                            this->FloatEncoder(encodedFloat, remappedDoubleAttrBuffer.data(), params, params.name);
                        }

                        // 只靠count就足以读取
                        params.binaryCount = encodedFloat.size() * sizeof(uint8_t);
                        outFloats[i] = encodedFloat;
                    }
                },
                threadNum);

        size_t currentPayloadCursor = 0;
        for (int i = 0; i < this->m_Params.attrParams.size(); i++) {
            payload.resize(currentPayloadCursor + outFloats[i].size());
            std::memcpy(payload.data() + currentPayloadCursor, outFloats[i].data(), outFloats[i].size());
            currentPayloadCursor += outFloats[i].size();
        }

        //std::vector<unsigned char> outFloat;
        //for (int i = 0; i < this->m_Params.attrParams.size(); i++)
        //{
        //    auto& attr = attrs->GetElement(i);
        //    auto& params = this->m_Params.attrParams[i];
        //    int dimension = params.dimension;
        //    int elementCount = params.elementCount;
        //    int valueCount = params.elementCount * params.dimension;

        //    // 部署remap
        //    std::vector<float> remappedFloatAttrBuffer;
        //    std::vector<double> remappedDoubleAttrBuffer;
        //    if (params.valueSize == sizeof(float))
        //    {
        //        remappedFloatAttrBuffer.resize(valueCount);
        //        ThreadPool::parallelFor(0, elementCount, [&](int start, int end) -> void {
        //            for (int j = start; j < end; j++)
        //            {
        //                igIndex remapIndex = (params.attachmentType == IG_POINT ? pointRemap[j] : cellRemap[j]);
        //                std::vector<double> values;
        //                attr.pointer->GetElement(j, values);

        //                for (int k = 0; k < params.dimension; k++)
        //                {
        //                    remappedFloatAttrBuffer[remapIndex * params.dimension + k] = values[k];
        //                }
        //            }
        //            });
        //    }
        //    else
        //    {
        //        remappedDoubleAttrBuffer.resize(valueCount);
        //        ThreadPool::parallelFor(0, elementCount, [&](int start, int end) -> void {
        //            for (int j = start; j < end; j++)
        //            {
        //                igIndex remapIndex = (params.attachmentType == IG_POINT ? pointRemap[j] : cellRemap[j]);
        //                std::vector<double> values;
        //                attr.pointer->GetElement(j, values);

        //                for (int k = 0; k < params.dimension; k++)
        //                {
        //                    remappedDoubleAttrBuffer[remapIndex * params.dimension + k] = values[k];
        //                }
        //            }
        //            });
        //    }

        //    // 编码
        //    std::vector<unsigned char> encodedFloat;
        //    if (params.valueSize == sizeof(float))
        //    {
        //        this->FloatEncoder(encodedFloat, remappedFloatAttrBuffer.data(), params);
        //    }
        //    else if (params.valueSize == sizeof(double))
        //    {
        //        this->FloatEncoder(encodedFloat, remappedDoubleAttrBuffer.data(), params);
        //    }

        //    // 只靠count就足以读取
        //    params.binaryCount = encodedFloat.size() * sizeof(uint8_t);
        //    outFloat.insert(outFloat.end(), encodedFloat.begin(), encodedFloat.end());
        //}

        //payload.resize(outFloat.size());
        //std::memcpy(payload.data(), outFloat.data(), outFloat.size());
    }

    // 和常规的delta encoder有点区别 产生的结果是每个cell的点的数量
    void DeltaEncoder(std::vector<uint32_t>& dest, std::vector<uint32_t> source) {
        IGsize destSize = source.size() - 1;
        dest.resize(destSize);
        ThreadPool::parallelFor(0, destSize, [&](int start, int end) -> void {
            int prev = source[start];
            for (int i = start; i < end; i++) {
                const uint32_t& cur = source[i + 1];
                dest[i] = cur - prev;
                prev = cur;
            }
        });
    }

    // import from meshoptimizer
    void encodeVByte(unsigned char*& data, unsigned int v) {
        // encode 32-bit value in up to 5 7-bit groups
        do {
            *data++ = (v & 127) | (v > 127 ? 128 : 0);
            v >>= 7;
        } while (v);
    }

    void RunLengthEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t> source) {
        // rle编码按理说不一定可以压缩 有时反而会导致数据变长 这里先不考虑
        // 由于采用了push_back 所以不用resize dest
        dest.resize(source.size() * sizeof(uint32_t) + 1024);
        unsigned char* startPointer = dest.data();
        unsigned char* destPointer = dest.data();


        int sizeCount = source.size();
        int blockSize = 1024; // 每个线程取得的串长度 单位byte

        int maxThreadSize = ceil(sizeCount / (float) blockSize); // 线程数量

        // 对于 AAAABBBBCCCCCC
        // 输出形如 0 4 8 14
        std::vector<std::vector<int>> threadRusult(
                maxThreadSize); // 线程结果 按顺序存储了每个线程检测到的多个数据变换点的位置

        ThreadPool::parallelFor(
                0, sizeCount, maxThreadSize,
                [&](int start, int end, int threadIndex) -> void {
                    std::vector<int>& curResult = threadRusult[threadIndex];

                    // 找上一个block中最后一个值 判断是否本block的第一个值就是变换点
                    bool isFirstValue = (start == 0);
                    uint32_t prev;

                    if (!isFirstValue) { prev = source[start - 1]; }

                    for (int i = start; i < end; i++) {
                        // 写入threadResult
                        if (!isFirstValue && source[i] != prev) {
                            curResult.push_back(i);
                            prev = source[i];
                        }
                        if (isFirstValue) {
                            curResult.push_back(0);
                            prev = source[i];

                            isFirstValue = false;
                        }
                    }
                },
                maxThreadSize);

        std::vector<int> offset; // 方便遍历
        for (int i = 0; i < threadRusult.size(); i++) {
            std::vector<int>& curResult = threadRusult[i];
            offset.insert(offset.end(), curResult.begin(), curResult.end());
        }
        offset.push_back(sizeCount);
        threadRusult.clear();

        // 编码
        // 先写入转义符号 用于声明转义
        //dest.push_back(minByte);
        int outputSize = 0;
        for (int i = 0; i < offset.size() - 1; i++) {
            const uint32_t curInt = source[offset[i]];
            unsigned int length = offset[i + 1] - offset[i];

            std::memcpy(destPointer, &curInt, sizeof(uint32_t)); // 写入cellsize
            destPointer += sizeof(uint32_t);
            this->encodeVByte(destPointer, length); // 写入长度 末尾block的高位为0
        }

        dest.resize(destPointer - startPointer);
    }

    // 对于types 直接执行rle
    void CellTypeEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t> source) {
        this->RunLengthEncoder(dest, source);
    }

    // 对于offset 执行 delta + rle
    void CellSizeEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t> source) {
        std::vector<uint32_t> delta;
        this->DeltaEncoder(delta, source);
        this->RunLengthEncoder(dest, delta);
    }

    void CellBufferEncoder(std::vector<unsigned char>& dest, std::vector<uint32_t>& source) {
        //std::vector<unsigned char> ibuf(meshopt_encodeIndexBufferBound(bufferSize, pointCount));
        //ibuf.resize(meshopt_encodeIndexBuffer(&ibuf[0], ibuf.size(), uintidbuffer.data(), bufferSize));

        //std::vector<char> binary(ibuf.size());
        //std::memcpy(binary.data(), ibuf.data(), ibuf.size());
        //this->m_BytestreamFile.write(binary.data(), binary.size());

        //FastPForLib::IntegerCODEC& codec = *FastPForLib::CODECFactory::getFromName("simdfastpfor256");
        //size_t compressedsize = dest.size();
        //codec.encodeArray(source.data(), source.size(), dest.data(),
        //    compressedsize);
        //dest.resize(compressedsize);
        //dest.shrink_to_fit();

        // 经过实验 meshopt的index buffer encoder在小规模mesh上略低于fastpfor(一种通用整数编码器)
        // 但在大规模mesh上显著超越fastpfor
        // meshopt的encoder以三角形为单元处理 其算法具有一定的对称性(指一次处理一个边和一个点)
        // 所以选择用padding来解决index buffer size不匹配问题 而不是大规模改造算法

        int pointCount = this->m_Params.geomParams.elementCount;

        // padding
        int bufferSize = source.size();
        int padding = (3 - bufferSize % 3) % 3;
        this->m_Params.topoParams.cellBufferPadding = padding;
        for (int i = 0; i < padding; i++) { source.push_back(i); }
        bufferSize += padding;
        // opt操作已经在外部完成

        dest.resize(IndexBufferCodec::encodeIndexBufferBound(bufferSize, pointCount));
        dest.resize(IndexBufferCodec::encodeIndexBuffer(&dest[0], dest.size(), source.data(), bufferSize));

        source.resize(bufferSize - padding);
    }

    // ---------------------------------------------------------------------
    // 下列与cache优化相关的代码改造自 meshoptimizer(license in thirdparty\meshoptimizer-0.22)
    static const size_t kCacheSizeMax = 16;
    static const size_t kValenceMax = 8;
    struct VertexScoreTable {
        float cache[1 + kCacheSizeMax];
        float live[1 + kValenceMax];
    };
    VertexScoreTable kVertexScoreTableStrip;

    struct CellAdjacency {
        unsigned int* counts;  // 顶点i -> 邻接live face数量
        unsigned int* offsets; // data的offset
        unsigned int* data;
        //       vertex0            vertex1             ...
        // |cell1, cell3, ...| cell2, cell9, ... |...
    };

    float VertexScore(int cachePosition, unsigned int liveCellCount) {
        assert(cachePosition >= -1 && cachePosition < int(kCacheSizeMax));
        unsigned int liveCellClamped = liveCellCount < this->kValenceMax ? liveCellCount : this->kValenceMax;
        return this->kVertexScoreTableStrip.cache[1 + cachePosition] +
               this->kVertexScoreTableStrip.live[liveCellClamped];
    }

    unsigned int GetNextCellDeadEnd(unsigned int& inputCursor, const unsigned char* emittedFlags, size_t cellCount) {
        // input order
        while (inputCursor < cellCount) {
            if (!emittedFlags[inputCursor]) return inputCursor;

            ++inputCursor;
        }

        return ~0u;
    }

    // -------------------------------------------------------------
    // 不定长offset

    void BuildHybirdCellAdjacency(CellAdjacency& adjacency, const unsigned int* sourceBuffer,
                                  const unsigned int* sourceOffset, size_t bufferSize, size_t pointCount,
                                  size_t cellCount, meshopt_Allocator& allocator) {
        adjacency.counts = allocator.allocate<unsigned int>(pointCount);
        adjacency.offsets = allocator.allocate<unsigned int>(pointCount);
        adjacency.data = allocator.allocate<unsigned int>(bufferSize);

        // fill cell counts
        memset(adjacency.counts, 0, pointCount * sizeof(unsigned int));

        // 计算顶点id的直方图 表达了mesh中每个顶点被cell引用的次数
        for (size_t i = 0; i < bufferSize; ++i) {
            assert(sourceBuffer[i] < pointCount);
            adjacency.counts[sourceBuffer[i]]++;
        }
        // 利用直方图计算data的offset
        unsigned int offset = 0;
        for (size_t i = 0; i < pointCount; ++i) {
            adjacency.offsets[i] = offset;
            offset += adjacency.counts[i];
        }
        assert(offset == bufferSize);

        // 填充data
        for (size_t i = 0; i < cellCount; i++) {
            for (size_t j = sourceOffset[i]; j < sourceOffset[i + 1]; j++) {
                adjacency.data[adjacency.offsets[sourceBuffer[j]]++] = unsigned(i);
            }
        }

        // fix offsets that have been disturbed by the previous pass
        for (size_t i = 0; i < pointCount; ++i) {
            assert(adjacency.offsets[i] >= adjacency.counts[i]);

            adjacency.offsets[i] -= adjacency.counts[i];
        }
    }

    void OptimizeHybirdCellVertexCache(unsigned int* destCellBuffer, unsigned int* destCellOffset,
                                       unsigned int* destCellRemap, const unsigned int* sourceCellBuffer,
                                       const unsigned int* sourceCellOffsets, size_t bufferSize, size_t offsetCount,
                                       size_t pointCount, size_t cellCount) {
        meshopt_Allocator allocator;

        // guard for empty meshes
        if (bufferSize == 0 || pointCount == 0) return;

        // support in-place optimization
        if (destCellBuffer == sourceCellBuffer) {
            unsigned int* temp_copy = allocator.allocate<unsigned int>(bufferSize);
            memcpy(temp_copy, sourceCellBuffer, bufferSize * sizeof(unsigned int));
            sourceCellBuffer = temp_copy;
        }
        if (destCellOffset == sourceCellOffsets) {
            unsigned int* temp_copy = allocator.allocate<unsigned int>(offsetCount);
            memcpy(temp_copy, sourceCellOffsets, offsetCount * sizeof(unsigned int));
            sourceCellOffsets = temp_copy;
        }

        unsigned int cache_size = 16;
        assert(cache_size <= kCacheSizeMax);

        // build adjacency information
        CellAdjacency adjacency = {};
        this->BuildHybirdCellAdjacency(adjacency, sourceCellBuffer, sourceCellOffsets, bufferSize, pointCount,
                                       cellCount, allocator);

        // live triangle counts; note, we alias adjacency.counts as we remove triangles after emitting them so the counts always match
        unsigned int* liveCellCount = adjacency.counts;

        // 已访问cell标记
        unsigned char* emittedFlags = allocator.allocate<unsigned char>(cellCount);
        memset(emittedFlags, 0, cellCount);

        // compute initial vertex scores
        float* vertexScores = allocator.allocate<float>(pointCount);

        for (size_t i = 0; i < pointCount; ++i) vertexScores[i] = this->VertexScore(-1, liveCellCount[i]);

        // compute cells scores
        float* cellScores = allocator.allocate<float>(cellCount);
        unsigned int maxIdPerCell = 0; // 用于稍后开双缓存
        for (size_t i = 0; i < cellCount; ++i) {
            cellScores[i] = 0;
            for (size_t j = sourceCellOffsets[i]; j < sourceCellOffsets[i + 1]; j++) {
                cellScores[i] += vertexScores[sourceCellBuffer[j]];
                maxIdPerCell = std::max(maxIdPerCell, sourceCellOffsets[i + 1] - sourceCellOffsets[i]);
            }
        }

        unsigned int* cacheHolder = new unsigned int[2 * (kCacheSizeMax + maxIdPerCell + 1)];
        unsigned int* cache = cacheHolder;
        unsigned int* cacheNew = cacheHolder + kCacheSizeMax + maxIdPerCell + 1;
        size_t cacheCount = 0;

        unsigned int currentCell = 0;
        unsigned int inputCursor = 1;

        unsigned int outputCell = 0;
        destCellOffset[0] = 0;
        while (currentCell != ~0u) {
            assert(outputCell < cellCount);
            size_t cacheWriteCursor = 0;
            for (size_t j = sourceCellOffsets[currentCell]; j < sourceCellOffsets[currentCell + 1]; j++) {
                destCellBuffer[destCellOffset[outputCell] + cacheWriteCursor] = sourceCellBuffer[j];
                cacheNew[cacheWriteCursor] = sourceCellBuffer[j];
                cacheWriteCursor++;
            }
            size_t currentCellVertexCount = sourceCellOffsets[currentCell + 1] - sourceCellOffsets[currentCell];
            destCellOffset[outputCell + 1] = destCellOffset[outputCell] + currentCellVertexCount;
            destCellRemap[currentCell] = outputCell;

            outputCell++;

            // update emitted flags
            emittedFlags[currentCell] = true;
            cellScores[currentCell] = 0;

            // 移动老cache的cell到cacheNew 但是当前cell的顶点不额外再次放入
            for (size_t i = 0; i < cacheCount; ++i) {
                unsigned int index = cache[i];
                cacheNew[cacheWriteCursor] = index;

                int indepIndex = 1;
                for (size_t j = sourceCellOffsets[currentCell]; j < sourceCellOffsets[currentCell + 1]; j++) {
                    if (index == sourceCellBuffer[j]) {
                        indepIndex = 0;
                        break;
                    }
                }
                cacheWriteCursor += indepIndex;
            }

            unsigned int* cacheTemp = cache;
            cache = cacheNew, cacheNew = cacheTemp;
            cacheCount = cacheWriteCursor > cache_size ? cache_size : cacheWriteCursor;

            // 从data中移除当前cell的vertex对当前cell的记录
            for (size_t k = 0; k < currentCellVertexCount; ++k) {
                unsigned int index = sourceCellBuffer[sourceCellOffsets[currentCell] + k];
                unsigned int* neighbors = &adjacency.data[0] + adjacency.offsets[index];
                size_t neighborsSize = adjacency.counts[index];
                for (size_t i = 0; i < neighborsSize; ++i) {
                    unsigned int cell = neighbors[i];
                    if (cell == currentCell) {
                        neighbors[i] = neighbors[neighborsSize - 1];
                        adjacency.counts[index]--;
                        break;
                    }
                }
            }

            unsigned int bestCell = ~0u;
            float bestCellScore = 0;

            // 更新被写入cache的顶点
            for (size_t i = 0; i < cacheWriteCursor; ++i) {
                unsigned int index = cache[i];

                // 如果当前顶点没有活动的邻接cell就跳过
                if (adjacency.counts[index] == 0) continue;

                // 已经写入cache的顶点数量cache_write可能多于cache size
                // 如果超出cache 其位置就记为-1
                int cachePosition = i >= cache_size ? -1 : int(i);

                // 通过计算顶点分数的方式 逐步更新cell分数
                float score = this->VertexScore(cachePosition, liveCellCount[index]);
                float scoreDiff = score - vertexScores[index];

                vertexScores[index] = score;

                // update scores of vertex triangles
                const unsigned int* neighborsBegin = &adjacency.data[0] + adjacency.offsets[index];
                const unsigned int* neighborsEnd = neighborsBegin + adjacency.counts[index];

                for (const unsigned int* it = neighborsBegin; it != neighborsEnd; ++it) {
                    unsigned int cell = *it;
                    assert(!emittedFlags[cell]);

                    float cellScore = cellScores[cell] + scoreDiff;
                    assert(cellScore > 0);

                    bestCell = bestCellScore < cellScore ? cell : bestCell;
                    bestCellScore = bestCellScore < cellScore ? cellScore : bestCellScore;

                    cellScores[cell] = cellScore;
                }
            }

            // step through input triangles in order if we hit a dead-end
            currentCell = bestCell;

            if (currentCell == ~0u) {
                currentCell = this->GetNextCellDeadEnd(inputCursor, &emittedFlags[0], cellCount);
            }
        }

        assert(inputCursor == cellCount);
        assert(outputCell == cellCount);
    }

    // -------------------------------------------------------------------
    // 固定offset

    void BuildCellAdjacency(CellAdjacency& adjacency, const unsigned int* sourceBuffer, size_t bufferSize,
                            size_t pointCount, size_t fixedCellSize, meshopt_Allocator& allocator) {
        size_t face_count = bufferSize / fixedCellSize;

        // allocate arrays
        adjacency.counts = allocator.allocate<unsigned int>(pointCount);
        adjacency.offsets = allocator.allocate<unsigned int>(pointCount);
        adjacency.data = allocator.allocate<unsigned int>(bufferSize);

        // fill triangle counts
        memset(adjacency.counts, 0, pointCount * sizeof(unsigned int));

        for (size_t i = 0; i < bufferSize; ++i) {
            assert(sourceBuffer[i] < pointCount);

            adjacency.counts[sourceBuffer[i]]++;
        }

        // fill offset table
        unsigned int offset = 0;

        for (size_t i = 0; i < pointCount; ++i) {
            adjacency.offsets[i] = offset;
            offset += adjacency.counts[i];
        }

        assert(offset == bufferSize);

        // fill cell data
        for (size_t i = 0; i < face_count; ++i) {
            for (size_t j = 0; j < fixedCellSize; j++) {
                adjacency.data[adjacency.offsets[sourceBuffer[i * fixedCellSize + j]]++] = unsigned(i);
            }
        }

        // fix offsets that have been disturbed by the previous pass
        for (size_t i = 0; i < pointCount; ++i) {
            assert(adjacency.offsets[i] >= adjacency.counts[i]);
            adjacency.offsets[i] -= adjacency.counts[i];
        }
    }

    void OptimizeCellVertexCache(unsigned int* destCellBuffer, unsigned int* destCellRemap,
                                 const unsigned int* sourceCellBuffer, size_t bufferSize, size_t pointCount,
                                 size_t fixedCellSize) {
        assert(bufferSize % fixedCellSize == 0);

        meshopt_Allocator allocator;

        // guard for empty meshes
        if (bufferSize == 0 || pointCount == 0) return;

        // support in-place optimization
        if (destCellBuffer == sourceCellBuffer) {
            unsigned int* indicesCopy = allocator.allocate<unsigned int>(bufferSize);
            memcpy(indicesCopy, sourceCellBuffer, bufferSize * sizeof(unsigned int));
            sourceCellBuffer = indicesCopy;
        }

        unsigned int cacheSize = 16;
        assert(cacheSize <= kCacheSizeMax);

        size_t cellCount = bufferSize / fixedCellSize;

        // build adjacency information
        CellAdjacency adjacency = {};
        BuildCellAdjacency(adjacency, sourceCellBuffer, bufferSize, pointCount, fixedCellSize, allocator);

        // live cell counts; note, we alias adjacency.counts as we remove cells after emitting them so the counts always match
        unsigned int* live_cells = adjacency.counts;

        // emitted flags
        unsigned char* emittedFlags = allocator.allocate<unsigned char>(cellCount);
        memset(emittedFlags, 0, cellCount);

        // compute initial vertex scores
        float* vertexScores = allocator.allocate<float>(pointCount);

        for (size_t i = 0; i < pointCount; ++i) vertexScores[i] = this->VertexScore(-1, live_cells[i]);

        // compute triangle scores
        float* cellScores = allocator.allocate<float>(cellCount);

        for (size_t i = 0; i < cellCount; ++i) {
            cellScores[i] = 0;
            for (size_t j = 0; j < fixedCellSize; j++) {
                cellScores[i] += vertexScores[sourceCellBuffer[i * fixedCellSize + j]];
            }
        }

        unsigned int* cacheHolder = new unsigned int[2 * (kCacheSizeMax + fixedCellSize + 1)];
        unsigned int* cache = cacheHolder;
        unsigned int* cacheNew = cacheHolder + kCacheSizeMax + fixedCellSize + 1;
        size_t cacheCount = 0;

        unsigned int currentCell = 0;
        unsigned int inputCursor = 1;

        unsigned int outputCell = 0;

        while (currentCell != ~0u) {
            assert(outputCell < cellCount);

            size_t cacheWrite = 0;
            for (size_t j = 0; j < fixedCellSize; j++) {
                destCellBuffer[outputCell * fixedCellSize + j] = sourceCellBuffer[currentCell * fixedCellSize + j];
                cacheNew[cacheWrite++] = sourceCellBuffer[currentCell * fixedCellSize + j];
            }
            destCellRemap[currentCell] = outputCell;
            outputCell++;

            // update emitted flags
            emittedFlags[currentCell] = true;
            cellScores[currentCell] = 0;

            // old cells
            for (size_t i = 0; i < cacheCount; ++i) {
                unsigned int index = cache[i];

                cacheNew[cacheWrite] = index;

                int indep_index = 1;
                for (size_t j = 0; j < fixedCellSize; j++) {
                    if (index == sourceCellBuffer[currentCell * fixedCellSize + j]) {
                        indep_index = 0;
                        break;
                    }
                }
                cacheWrite += indep_index;
            }

            unsigned int* cache_temp = cache;
            cache = cacheNew, cacheNew = cache_temp;
            cacheCount = cacheWrite > cacheSize ? cacheSize : cacheWrite;

            for (size_t k = 0; k < fixedCellSize; ++k) {
                unsigned int index = sourceCellBuffer[currentCell * fixedCellSize + k];

                unsigned int* neighbors = &adjacency.data[0] + adjacency.offsets[index];
                size_t neighborsSize = adjacency.counts[index];

                for (size_t i = 0; i < neighborsSize; ++i) {
                    unsigned int cell = neighbors[i];

                    if (cell == currentCell) {
                        neighbors[i] = neighbors[neighborsSize - 1];
                        adjacency.counts[index]--;
                        break;
                    }
                }
            }

            unsigned int bestCell = ~0u;
            float bestScore = 0;

            // update cache remappedPointBuffer, vertex scores and triangle scores, and find next best triangle
            for (size_t i = 0; i < cacheWrite; ++i) {
                unsigned int index = cache[i];

                // no need to update scores if we are never going to use this vertex
                if (adjacency.counts[index] == 0) continue;

                int cachePosition = i >= cacheSize ? -1 : int(i);

                // update vertex score
                float score = this->VertexScore(cachePosition, live_cells[index]);
                float scoreDiff = score - vertexScores[index];

                vertexScores[index] = score;

                // update scores of vertex triangles
                const unsigned int* neighborsBegin = &adjacency.data[0] + adjacency.offsets[index];
                const unsigned int* neighborsEnd = neighborsBegin + adjacency.counts[index];

                for (const unsigned int* it = neighborsBegin; it != neighborsEnd; ++it) {
                    unsigned int cell = *it;
                    assert(!emittedFlags[cell]);

                    float cellScore = cellScores[cell] + scoreDiff;
                    assert(cellScore > 0);

                    bestCell = bestScore < cellScore ? cell : bestCell;
                    bestScore = bestScore < cellScore ? cellScore : bestScore;

                    cellScores[cell] = cellScore;
                }
            }

            // step through input cells in order if we hit a dead-end
            currentCell = bestCell;

            if (currentCell == ~0u) {
                currentCell = this->GetNextCellDeadEnd(inputCursor, &emittedFlags[0], cellCount);
            }
        }

        assert(inputCursor == cellCount);
        assert(outputCell == cellCount);
    }
};

IGAME_NAMESPACE_END
#endif