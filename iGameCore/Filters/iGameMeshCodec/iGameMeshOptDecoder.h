#ifndef MeshOptDecoder_h
#define MeshOptDecoder_h

#include "iGameMacro.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshOptCodec.h"

#include "iGameMeshDecoderAdapter.h"
#include <future>
#include "iGameMeshCodecLZMA.h"

IGAME_NAMESPACE_BEGIN
class MeshOptDecoder : public MeshOptCodec {
public:
    MeshOptDecoder(
        std::ifstream& bytestreamFile,
        MeshOptParameters& params) :
        MeshOptCodec(params),
        m_BytestreamFile(bytestreamFile)
    {};

    DataObject::Pointer Execute()
    {
        PayloadBuffer buf;
        while (true) {
            ReadBuf(this->m_BytestreamFile, &buf);
            PayloadType type = buf.type;

            PayloadBuffer bufDecompressed;
            MeshCodecLZMA::Decompress(bufDecompressed, buf);

			float progress = 0.0;

            switch (type)
            {
            case PayloadType::kParameterSet:
            {
                // 虽然这里没有控制顺序 但编码时应严格要求首先写入params
                this->ParamsDecoder(bufDecompressed);
				progress += 0.2;
                break;
            }
            case PayloadType::kGeometryBrick:
            {
                this->GeomDecoder(bufDecompressed);
                progress += 0.2;
                break;
            }
            case PayloadType::kAttributeBrick:
            {
                this->AttrDecoder(bufDecompressed);
                progress += 0.2;
                break;
            }
            case PayloadType::kTopologyBrick:
            {
                this->TopoDecoder(bufDecompressed);
                progress += 0.2;
                break;
            }
            default:
                break;
            }

            this->m_CallBack(progress);

            //switch (buf.type)
            //{
            //case PayloadType::kParameterSet:
            //{
            //    // 虽然这里没有控制顺序 但编码时应严格要求首先写入params
            //    this->ParamsDecoder(buf);
            //    break;
            //}
            //case PayloadType::kGeometryBrick:
            //{
            //    this->GeomDecoder(buf);
            //    break;
            //}
            //case PayloadType::kAttributeBrick:
            //{
            //    this->AttrDecoder(buf);
            //    break;
            //}
            //case PayloadType::kTopologyBrick:
            //{
            //    this->TopoDecoder(buf);
            //    break;
            //}
            //default:
            //    break;
            //}

            // at end of file (or other error), flush decoder
            if (!this->m_BytestreamFile)
            {
                break;
            }
        }

        this->m_CallBack(1.0);

        return this->m_DecoderAdapter->GetDataObj();
    }

    template<typename Functor, typename... Args>
    void SetUpdateProgress(Functor&& functor, Args&&... args) {
        this->m_CallBack =
            std::bind(std::forward<Functor>(functor), std::forward<Args>(args)..., std::placeholders::_1);
    }

    void ParamsDecoder(PayloadBuffer& buf)
    {
        IGsize staticSize = sizeof(MeshOptParametersWithoutAttr);

        // 读取静态数据
        MeshOptParametersWithoutAttr paramsWoAttr;
        std::memcpy(&paramsWoAttr, buf.data(), staticSize);

        this->m_Params.meshType = paramsWoAttr.meshType;
        this->m_Params.structuredMeshParams = paramsWoAttr.structuredMeshParams;
        
        this->m_Params.geomParams = paramsWoAttr.geomParams;
        this->m_Params.topoParams = paramsWoAttr.topoParams;

        this->m_Params.attrCount = paramsWoAttr.attrCount;

        // 读取动态数据
        this->m_Params.attrParams.resize(paramsWoAttr.attrCount);
        IGsize dynamicSize = paramsWoAttr.attrCount * sizeof(MeshOptAttrParameters);

        std::memcpy(this->m_Params.attrParams.data(), buf.data() + staticSize, dynamicSize);

        // 初始adapter
        this->m_DecoderAdapter = new MeshDecoderAdapter(this->m_Params.meshType);
    }

    // dest需要在外部先开辟好空间
    void FloatDecoder(void* dest,
        const std::vector<unsigned char>& source,
        const MeshOptFloatParameters& floatParams)
    {
        std::vector<float> floatBuffer;
        std::vector<double> doubleBuffer;
        
        bool floatFlag = false;
        bool doubleFlag = false;

        IGsize valueCount = floatParams.elementCount * floatParams.dimension;
        
        switch (floatParams.quantMode)
        {
        case QuantMode::None:
        {
            IGsize vertexSize = floatParams.valueSize * floatParams.dimension;
            if (floatParams.valueSize == sizeof(float))
            {
                floatBuffer.resize(valueCount);
                floatFlag = true;

                meshopt_decodeVertexBuffer(
                    floatBuffer.data(),
                    floatParams.elementCount,
                    vertexSize,
                    source.data(),
                    source.size()
                );
            }
            else
            {
                doubleBuffer.resize(valueCount);
                doubleFlag = true;

                meshopt_decodeVertexBuffer(
                    doubleBuffer.data(),
                    floatParams.elementCount,
                    vertexSize,
                    source.data(),
                    source.size()
                );
            }

            break;
        }
        case QuantMode::FP16:
        {
            // 这种情况的输出只有float
            floatBuffer.resize(valueCount); // 开多了没问题 别开少了就行
            floatFlag = true;

            // 由于量化是两个float并一个存 导致不一定输出还是原始dimension的倍数
            // 所以只好分离的编码 而不能成组编码
            IGsize vertexSize = sizeof(float); 
            size_t quantValueCount = (size_t)ceil(valueCount / 2.);

            // 先把量化后的序列读到floatBuffer
            meshopt_decodeVertexBuffer(
                floatBuffer.data(),
                quantValueCount,
                vertexSize,
                source.data(),
                source.size()
            );

            // 反量化
            std::vector<float> dequantBuffer(valueCount);
            
            const size_t blockSize = 65536;
            ThreadPool::Pointer tp = ThreadPool::Instance();
            int tpCount = (size_t)ceil(quantValueCount / (double)blockSize);
            std::vector<std::future<void>> tpResult(tpCount);

            for (int i = 0, tpCursor = 0; i < quantValueCount; i += blockSize, tpCursor++) // 这里quantiValueCount是quantize后的序列的元素数量
            {
                size_t length = std::min(quantValueCount - i, blockSize);
                tpResult[tpCursor] = tp->Commit([&](int start, int end) ->void {
                    for (int j = start; j < end; j++)
                    {
                        // 读取一个值
                        uint32_t group;
                        std::memcpy(&group, &floatBuffer[j], sizeof(uint32_t));
                        int shift = sizeof(unsigned short) * 8;

                        float a = meshopt_dequantizeHalf((unsigned short)(group >> shift));
                        float b = meshopt_dequantizeHalf((unsigned short)(group & ((1 << shift) - 1)));
                        
                        dequantBuffer[j * 2] = a;
                        if (j * 2 + 1 != valueCount)
                        {
                            dequantBuffer[j * 2 + 1] = b;

                        }
                    }
                }, i, i + length);
            }

            for (int i = 0; i < tpCount; i++) { tpResult[i].wait(); }

            // 转移结果
            floatBuffer.clear();
            floatBuffer.resize(valueCount);
            std::memcpy(floatBuffer.data(), dequantBuffer.data(), valueCount * sizeof(float));

            break;
        }
        case QuantMode::Float:
        {
            // 这种情况的输出只有float
            IGsize vertexSize = sizeof(float) * floatParams.dimension;
            floatFlag = true;

            floatBuffer.resize(valueCount);
            meshopt_decodeVertexBuffer(
                floatBuffer.data(),
                floatParams.elementCount,
                vertexSize,
                source.data(),
                source.size()
            );

            break;
        }
        default:
            break;
        }

        // 写入dest
        // 两种普通情形 实际产生的数据类型和dest对应
        if (floatFlag && floatParams.valueSize == sizeof(float))
        {
            std::memcpy(static_cast<float*>(dest), floatBuffer.data(), valueCount * sizeof(float));
        }
        else if (doubleFlag && floatParams.valueSize == sizeof(double))
        {
            std::memcpy(static_cast<double*>(dest), doubleBuffer.data(), valueCount * sizeof(double));
        }

        // 特殊情形 只有float数据且实际需要的是double 
        // 这种错配是量化导致的 所以没有只有double但实际需要float的情形
        
        if (floatFlag && floatParams.valueSize == sizeof(double))
        {
            ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
                for (int i = start; i < end; i++)
                {
                    static_cast<double*>(dest)[i] = floatBuffer[i];
                }
            });
        }

        return;
        //std::vector<float> convertFloatBuffer;
        //std::vector<double> convertDoubleBuffer;

        //IGsize valueCount = floatParams.elementCount * floatParams.dimension;

        //switch (floatParams.quantMode)
        //{
        //case QuantMode::None:
        //{
        //    IGsize elementSize = floatParams.valueSize * floatParams.dimension;
        //    int ret;
        //    if (floatParams.valueSize == sizeof(float))
        //    {
        //        convertFloatBuffer.resize(valueCount);
        //        ret = meshopt_decodeVertexBuffer(
        //            convertFloatBuffer.data(),
        //            floatParams.elementCount,
        //            elementSize,
        //            source.data(),
        //            source.size()
        //        );
        //    }
        //    else if (floatParams.valueSize == sizeof(double))
        //    {
        //        convertDoubleBuffer.resize(valueCount);
        //        ret = meshopt_decodeVertexBuffer(
        //            convertDoubleBuffer.data(),
        //            floatParams.elementCount,
        //            elementSize,
        //            source.data(),
        //            source.size()
        //        );
        //    }

        //    break;
        //}
        //case QuantMode::FP16:
        //{
        //    IGsize elementSize = sizeof(unsigned short) * floatParams.dimension;
        //    std::vector<unsigned short> fp16Buffer(valueCount);

        //    convertFloatBuffer.resize(
        //        meshopt_decodeVertexBuffer(
        //            fp16Buffer.data(),
        //            floatParams.elementCount,
        //            elementSize,
        //            source.data(),
        //            source.size()
        //        )
        //    );

        //    // 受到fp16精度限制 返回的只能是vector<float>
        //    std::transform(
        //        convertFloatBuffer.begin(),
        //        convertFloatBuffer.end(),
        //        fp16Buffer.begin(),
        //        [&](unsigned short x) {return meshopt_dequantizeHalf(x); }
        //    );
        //    break;
        //}
        //case QuantMode::Float:
        //{
        //    // 没有dequantize 这种的函数，GG
        //    break;
        //}
        //default:
        //    break;
        //}

        //// 定点数为不可逆操作 所以无需处理

        //// 输出 float一定可以取得 但是double不一定
        //if (floatParams.valueSize == sizeof(float))
        //{
        //    memcpy(static_cast<float*>(dest),
        //        convertFloatBuffer.data(),
        //        convertFloatBuffer.size() * sizeof(float)
        //    );
        //}
        //else if (floatParams.valueSize == sizeof(double))
        //{
        //    if (convertDoubleBuffer.empty())
        //    {
        //        double* destPtr = static_cast<double*>(dest);

        //        std::transform(
        //            destPtr,
        //            destPtr + valueCount,
        //            convertFloatBuffer.begin(),
        //            [&](float x) {return static_cast<double>(x); }
        //        );
        //    }
        //    else
        //    {
        //        memcpy(static_cast<double*>(dest),
        //            convertDoubleBuffer.data(),
        //            convertDoubleBuffer.size() * sizeof(double)
        //        );
        //    }
        //}
    }

    void GeomDecoder(PayloadBuffer& buf)
    {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        IGsize bufferSize = this->m_Params.geomParams.elementCount * this->m_Params.geomParams.dimension;
        std::vector<float> decodedFloat(bufferSize);
        this->FloatDecoder(decodedFloat.data(), uCharBuffer, this->m_Params.geomParams);

        this->m_DecoderAdapter->SetPointBuffer(decodedFloat);
    }

    void AttrDecoder(PayloadBuffer& buf)
    {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());
        
        int binaryCursor = 0;
        for (int i = 0; i < this->m_Params.attrParams.size(); i++)
        {
            auto& params = this->m_Params.attrParams[i];
            int dimension = params.dimension;
            int elementCount = params.elementCount;
            int valueCount = params.elementCount * params.dimension;
            
            std::vector<unsigned char> inputBuffer(
                uCharBuffer.begin() + binaryCursor,
                uCharBuffer.begin() + binaryCursor + params.binaryCount
            );

            binaryCursor += params.binaryCount;
            
            FloatArray::Pointer decodedFloat{ nullptr };
            DoubleArray::Pointer decodedDouble{ nullptr };
            if (params.valueSize == sizeof(float))
            {
                decodedFloat = FloatArray::New();
                decodedFloat->Resize(valueCount);
                this->FloatDecoder(decodedFloat->RawPointer(), inputBuffer, params);
            }
            else if (params.valueSize == sizeof(double))
            {
                decodedDouble = DoubleArray::New();
                decodedDouble->Resize(valueCount);
                this->FloatDecoder(decodedDouble->RawPointer(), inputBuffer, params);
            }

            // 设置attribute
            ArrayObject::Pointer data =
                (params.valueSize == sizeof(float) ?
                    DynamicCast<ArrayObject>(decodedFloat) :
                    DynamicCast<ArrayObject>(decodedDouble));
            data->SetName(params.name);
            data->SetDimension(params.dimension);

            this->m_DecoderAdapter->AddAttribute(params.type, params.attachmentType, data);
        }
    }

    unsigned int decodeVByte(unsigned char*& data)
    {
        unsigned char lead = *data++;

        // fast path: single byte
        if (lead < 128)
            return lead;

        // slow path: up to 4 extra bytes
        // note that this loop always terminates, which is important for malformed data
        unsigned int result = lead & 127;
        unsigned int shift = 7;

        for (int i = 0; i < 4; ++i)
        {
            unsigned char group = *data++;
            result |= unsigned(group & 127) << shift;
            shift += 7;

            if (group < 128)
                break;
        }

        return result;
    }

    void RunLengthDecoder(std::vector<uint32_t>& dest, unsigned char* source, int sourceCount)
    {
        unsigned char* start = source;

        while(true)
        {
            // 读数据
            uint32_t readInt;
            std::memcpy(&readInt, source, sizeof(uint32_t));
            source += sizeof(uint32_t);

            // 读长度
            unsigned int length = this->decodeVByte(source);

            // 写入
            for (int j = 0; j < length; j++)
            {
                dest.push_back(readInt);
            }

            if (source - start == sourceCount)
            {
                break;
            }
        }

        return;
    }
    
    void CellBufferDecoder(std::vector<uint32_t>& dest, const unsigned char* source, int sourceCount, int bufferPadding)
    {
        int resib = IndexBufferCodec::decodeIndexBuffer(dest.data(), dest.size(), source, sourceCount);
        assert(resib == 0);
        dest.resize(dest.size() - bufferPadding);
    }

    void CellTypeDecoder(std::vector<uint32_t>& dest, unsigned char* source, int sourceCount)
    {
        this->RunLengthDecoder(dest, source, sourceCount);
    }

    void CellSizeDecoder(std::vector<uint32_t>& dest, unsigned char* source, int sourceCount)
    {
        this->RunLengthDecoder(dest, source, sourceCount);
    }

    void IndexNOffsetDecoder(
        std::vector<unsigned char>& inputTopo,
        int& cursor,

        std::vector<unsigned int>& outIndices,
        const IGsize indicesBinaryCount,
        const IGsize indicesBufferSize,
        const int indicesBufferPadding,

        int fixedCellSize,
        const IGsize cellSizeBinaryCount,
        std::vector<unsigned int>& outCellSizes

    ) {
        // 解码cellbuffer
        outIndices.resize(indicesBufferSize + indicesBufferPadding);
        this->CellBufferDecoder(
            outIndices,
            inputTopo.data() + cursor,
            indicesBinaryCount,
            indicesBufferPadding
        );
        cursor += indicesBinaryCount;

        // 解码size
        if (fixedCellSize == -1)
        {
            this->CellSizeDecoder(
                outCellSizes,
                inputTopo.data() + cursor,
                cellSizeBinaryCount
            );
            cursor += cellSizeBinaryCount;
        }
    }

    void TopoDecoder(PayloadBuffer& buf)
    {
        std::vector<unsigned char> inputTopo(buf.size());
        std::memcpy(inputTopo.data(), buf.data(), buf.size());

        int inputCursor = 0;
        
        if (this->m_Params.topoParams.isSecondaryIndex)
        {
            std::vector<unsigned int> volume2facesIndex;
            std::vector<unsigned int> volume2facesSize;
            std::vector<unsigned int> face2pointsIndex;
            std::vector<unsigned int> face2pointsSize;

            // 解码 面 -> 顶点
            IndexNOffsetDecoder
            (
                inputTopo,
                inputCursor,
                face2pointsIndex,
                this->m_Params.topoParams.bottomCellBufferBinaryCount,
                this->m_Params.topoParams.bottomCellBufferSize,
                this->m_Params.topoParams.bottomCellBufferPadding,
                -1,
                this->m_Params.topoParams.bottomCellSizeBinaryCount,
                face2pointsSize
            );

            // 解码 体 -> 面
            IndexNOffsetDecoder
            (
                inputTopo,
                inputCursor,
                volume2facesIndex,
                this->m_Params.topoParams.topCellBufferBinaryCount,
                this->m_Params.topoParams.topCellBufferSize,
                this->m_Params.topoParams.topCellBufferPadding,
                -1,
                this->m_Params.topoParams.topCellSizeBinaryCount,
                volume2facesSize
            );

            this->m_DecoderAdapter->AddSecondaryIndexCells(
                volume2facesIndex,
                volume2facesSize,
                face2pointsIndex,
                face2pointsSize
            );
        }
        else
        {
            std::vector<unsigned int> volume2pointsIndex;
            std::vector<unsigned int> volume2pointsSize;
            std::vector<unsigned int> outCellTypes;
            int fixedCellSize = this->m_Params.topoParams.fixedCellSize;
            bool isFixedCellSize = fixedCellSize != -1;

            if (isFixedCellSize)
            {
                IndexNOffsetDecoder
                (
                    inputTopo,
                    inputCursor,
                    volume2pointsIndex,
                    this->m_Params.topoParams.topCellBufferBinaryCount,
                    this->m_Params.topoParams.topCellBufferSize,
                    this->m_Params.topoParams.topCellBufferPadding,
                    fixedCellSize,
                    this->m_Params.topoParams.topCellSizeBinaryCount, // 后两个参数无实际作用
                    volume2pointsSize
                );
            }
            else
            {
                IndexNOffsetDecoder
                (
                    inputTopo,
                    inputCursor,
                    volume2pointsIndex,
                    this->m_Params.topoParams.topCellBufferBinaryCount,
                    this->m_Params.topoParams.topCellBufferSize,
                    this->m_Params.topoParams.topCellBufferPadding,
                    -1,
                    this->m_Params.topoParams.topCellSizeBinaryCount,
                    volume2pointsSize
                );
            }

            // 如果是非结构化网格 还要考虑types
            if (this->m_Params.meshType == IG_UNSTRUCTURED_MESH)
            {
                this->CellTypeDecoder(
                    outCellTypes,
                    inputTopo.data() + inputCursor,
                    this->m_Params.topoParams.cellTypeBinaryCount
                );
                inputCursor += this->m_Params.topoParams.cellTypeBinaryCount;
            }

            // 写入网格
            
            bool unstructuredFlag = this->m_Params.meshType == IG_UNSTRUCTURED_MESH;
            bool structuredFlag = this->m_Params.meshType == IG_STRUCTURED_MESH;

            // 非结构化网格
            if (unstructuredFlag)
            {
                if (isFixedCellSize)
                {
                    this->m_DecoderAdapter->AddUnstructuredFixedCells(volume2pointsIndex, fixedCellSize, outCellTypes);
                }
                else
                {
                    this->m_DecoderAdapter->AddUnstructuredPolyCells(volume2pointsIndex, volume2pointsSize, outCellTypes);
                }
            }
            else if (structuredFlag)
            {
                this->m_DecoderAdapter->AddStructureCells(volume2pointsIndex, this->m_Params.structuredMeshParams.axisSize);
            }
            else
            {
                if (isFixedCellSize)
                {
                    this->m_DecoderAdapter->AddSameTypeFixedCells(volume2pointsIndex, fixedCellSize);
                }
                else
                {
                    this->m_DecoderAdapter->AddSameTypePolyCells(volume2pointsIndex, volume2pointsSize);
                }
            }
        }
    }

    //void TopoDecoder(PayloadBuffer& buf)
    //{
    //    std::vector<unsigned int> outCellBuffer;
    //    std::vector<unsigned int> outCellSize;
    //    std::vector<unsigned int> outCellTypes;
    //    
    //    int fixedCellSize = this->m_Params.topoParams.fixedCellSize;
    //    //std::vector<unsigned int> fastpforBuffer(buf.size() / sizeof(uint32_t));

    //    std::vector<unsigned char> inputTopo(buf.size());
    //    std::memcpy(inputTopo.data(), buf.data(), buf.size());
    //    int inputCursor = 0;
    //    // 解码cellbuffer
    //    outCellBuffer.resize(this->m_Params.topoParams.cellBufferSize + this->m_Params.topoParams.cellBufferPadding);
    //    this->CellBufferDecoder(
    //        outCellBuffer, 
    //        inputTopo.data() + inputCursor,
    //        this->m_Params.topoParams.cellBufferBinaryCount
    //    );
    //    inputCursor += this->m_Params.topoParams.cellBufferBinaryCount;

    //    // 解码size
    //    if (fixedCellSize == -1)
    //    {
    //        this->CellSizeDecoder(
    //            outCellSize,
    //            inputTopo.data() + inputCursor,
    //            this->m_Params.topoParams.cellSizeBinaryCount
    //        );
    //        inputCursor += this->m_Params.topoParams.cellSizeBinaryCount;
    //    }

    //    // 如果是非结构化网格 还要考虑types
    //    if (this->m_Params.meshType == IG_UNSTRUCTURED_MESH)
    //    {
    //        this->CellTypeDecoder(
    //            outCellTypes,
    //            inputTopo.data() + inputCursor,
    //            this->m_Params.topoParams.cellTypeBinaryCount
    //        );
    //        inputCursor += this->m_Params.topoParams.cellTypeBinaryCount;
    //    }

    //    // 写入网格
    //    bool sizeFlag = fixedCellSize != -1;
    //    bool unstructuredFlag = this->m_Params.meshType == IG_UNSTRUCTURED_MESH;
    //    bool structuredFlag = this->m_Params.meshType == IG_STRUCTURED_MESH;

    //    // 非结构化网格
    //    if (unstructuredFlag)
    //    {
    //        if (sizeFlag)
    //        {
    //            this->m_DecoderAdapter->AddUnstructuredFixedCells(outCellBuffer, fixedCellSize, outCellTypes);
    //        }
    //        else
    //        {
    //            this->m_DecoderAdapter->AddUnstructuredPolyCells(outCellBuffer, outCellSize, outCellTypes);
    //        }
    //    }
    //    else if (structuredFlag)
    //    {
    //        this->m_DecoderAdapter->AddStructureCells(outCellBuffer, this->m_Params.structuredMeshParams.axisSize);
    //    }
    //    else
    //    {
    //        if (sizeFlag)
    //        {
    //            this->m_DecoderAdapter->AddSameTypeFixedCells(outCellBuffer, fixedCellSize);
    //        }
    //        else
    //        {
    //            this->m_DecoderAdapter->AddSameTypePolyCells(outCellBuffer, outCellSize);
    //        }
    //    }
    //}

private:
    std::ifstream& m_BytestreamFile;
    MeshDecoderAdapter* m_DecoderAdapter;
    std::function<void(double)> m_CallBack;
};
IGAME_NAMESPACE_END
#endif