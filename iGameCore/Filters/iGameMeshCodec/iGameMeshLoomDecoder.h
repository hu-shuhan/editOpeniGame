#ifndef MeshLoomDecoder_h
#define MeshLoomDecoder_h

#include "iGameFilter.h"
#include "iGameMacro.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshLoomCodec.h"
#include "iGameMeshFloatCodec.h"
#include "iGameMeshDecoderAdapter.h"
#include <future>
#include "iGameMeshCodecLZMA.h"

IGAME_NAMESPACE_BEGIN
class MeshLoomDecoder : public MeshLoomCodec {
public:
    MeshLoomDecoder(std::string readFilePath) :
        m_ReadFilePath(readFilePath)
    {};

    DataObject::Pointer Execute()
    {
        if (!m_ReadFilePath.empty() && !this->OpenStream(m_ReadFilePath)) { return nullptr; }

        PayloadBuffer buf;
        while (true) {
            ReadBuf(this->m_BytestreamFile, &buf);
            PayloadType type = buf.type;

            PayloadBuffer bufDecompressed;
            MeshCodecLZMA::Decompress(bufDecompressed, buf);

            switch (type)
            {
            case PayloadType::kParameterSet:
            {
                // 虽然这里没有控制顺序 但编码时应严格要求首先写入params
                this->ParamsDecoder(bufDecompressed);

                // 初始adapter
                this->m_DecoderAdapter = new MeshDecoderAdapter(this->m_codecParams.meshType);
                break;
            }
            case PayloadType::kGeometryBrick:
            {
                this->GeomDecoder(bufDecompressed);
                break;
            }
            case PayloadType::kAttributeBrick:
            {
                this->AttrDecoder(bufDecompressed);
                break;
            }
            case PayloadType::kTopologyBrick:
            {
                this->TopoDecoder(bufDecompressed);
                break;
            }
            default:
                break;
            }

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

        UpdateProgress(1.0);

        return this->m_DecoderAdapter->GetDataObj();
    }

private:
    std::string m_ReadFilePath;
    std::ifstream m_BytestreamFile;
    MeshDecoderAdapter* m_DecoderAdapter;
    float m_DecompressProgress = 0.0f;

    ProgressObserver* m_Progress{ nullptr };
    void UpdateProgress(double p) {
        if (m_Progress) {
            m_Progress->UpdateProgress(p);
        }
        else {
            m_Progress = ProgressObserver::Instance();
        }
    }

    void ParamsDecoder(PayloadBuffer& buf)
    {
        IGsize staticSize = sizeof(ParametersWoAttr);

        // 读取静态数据
        ParametersWoAttr paramsWoAttr;
        std::memcpy(&paramsWoAttr, buf.data(), staticSize);

        this->m_codecParams.meshType = paramsWoAttr.meshType;
        this->m_codecParams.structuredMeshParams = paramsWoAttr.structuredMeshParams;

        this->m_codecParams.geomParams = paramsWoAttr.geomParams;
        this->m_codecParams.topoParams = paramsWoAttr.topoParams;

        this->m_codecParams.attrCount = paramsWoAttr.attrCount;

        // 读取动态数据
        this->m_codecParams.attrParams.resize(paramsWoAttr.attrCount);
        IGsize dynamicSize = paramsWoAttr.attrCount * sizeof(AttrParameters);

        m_DecompressProgress += 0.1;
        UpdateProgress(m_DecompressProgress);

        std::memcpy(this->m_codecParams.attrParams.data(), buf.data() + staticSize, dynamicSize);

        m_DecompressProgress += 0.1;
        UpdateProgress(m_DecompressProgress);
    }

    void GeomDecoder(PayloadBuffer& buf)
    {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        m_DecompressProgress += 0.05;
        UpdateProgress(m_DecompressProgress);

        IGsize bufferSize = this->m_codecParams.geomParams.elementCount * this->m_codecParams.geomParams.dimension;
        std::vector<float> decodedFloat(bufferSize);
        
        MeshOptFloatCodec::FloatDecoder(decodedFloat, uCharBuffer, this->m_codecParams.geomParams);

        m_DecompressProgress += 0.15;
        UpdateProgress(m_DecompressProgress);

        this->m_DecoderAdapter->SetPointBuffer(decodedFloat);
    }

    //void AttrDecoder(PayloadBuffer& buf)
    //{
    //    std::vector<unsigned char> uCharBuffer(buf.size());
    //    std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

    //    int binaryCursor = 0;
    //    for (int i = 0; i < this->m_codecParams.attrParams.size(); i++)
    //    {
    //        auto& params = this->m_codecParams.attrParams[i];
    //        int dimension = params.dimension;
    //        int elementCount = params.elementCount;
    //        int valueCount = params.elementCount * params.dimension;

    //        std::vector<unsigned char> inputBuffer(
    //            uCharBuffer.begin() + binaryCursor,
    //            uCharBuffer.begin() + binaryCursor + params.binaryCount
    //        );

    //        binaryCursor += params.binaryCount;

    //        std::vector<float> floats;
    //        std::vector<double> doubles;


    //        auto processType = [&](auto& container, auto valueType) {
    //            using ValueType = decltype(valueType);
    //            using ArrayType = FlatArray<ValueType>;

    //            auto decoded = ArrayType::New();
    //            decoded->Resize(valueCount);

    //            MeshOptFloatCodec::FloatDecoder(container, inputBuffer, this->m_codecParams.attrParams[i]);
    //            memcpy(decoded->RawPointer(), container.data(), valueCount * sizeof(ValueType));

    //            decoded->SetDimension(params.dimension);
    //            decoded->SetName(params.name);
    //            this->m_DecoderAdapter->AddAttribute(params.type, params.attachmentType, decoded);
    //            };

    //        // 调用部分
    //        if (params.valueSize == sizeof(float)) {
    //            processType(floats, float{});
    //        }
    //        else if (params.valueSize == sizeof(double)) {
    //            processType(doubles, double{});
    //        }

    //        /*
    //        FlatArray<float>::Pointer decodedFloat{ nullptr };
    //        FlatArray<double>::Pointer decodedDouble{ nullptr };

    //        if (params.valueSize == sizeof(float))
    //        {
    //            decodedFloat = FlatArray<float>::New();
    //            decodedFloat->Resize(valueCount);

    //            MeshOptFloatCodec::FloatDecoder(floats, inputBuffer, this->m_codecParams.attrParams[i]);
    //            memcpy(decodedFloat->RawPointer(), floats.data(), valueCount * sizeof(float));
    //            decodedFloat->SetDimension(params.dimension);
    //            decodedFloat->SetName(params.name);
    //            this->m_DecoderAdapter->AddAttribute(params.type, params.attachmentType, decodedFloat);

    //            //this->FloatDecoder(decodedFloat->RawPointer(), inputBuffer, params);
    //        }
    //        else if (params.valueSize == sizeof(double))
    //        {
    //            decodedDouble = FlatArray<double>::New();
    //            decodedDouble->Resize(valueCount);

    //            MeshOptFloatCodec::FloatDecoder(doubles, inputBuffer, this->m_codecParams.attrParams[i]);
    //            memcpy(decodedDouble->RawPointer(), doubles.data(), valueCount * sizeof(double));
    //            decodedDouble->SetDimension(params.dimension);
    //            decodedDouble->SetName(params.name);
    //            this->m_DecoderAdapter->AddAttribute(params.type, params.attachmentType, decodedDouble);
    //            //this->FloatDecoder(decodedDouble->RawPointer(), inputBuffer, params);
    //        }
    //        */


    //        m_DecompressProgress += 0.2 * (i * 1.0 / this->m_codecParams.attrParams.size());
    //        UpdateProgress(m_DecompressProgress);
    //    }
    //}

    void AttrDecoder(PayloadBuffer& buf)
    {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());
        int binaryCursor = 0;

        // 用于暂存所有线程产生的属性数据
        struct AttributeInfo {
            IGenum type;
            IGenum attachmentType;
            ArrayObject::Pointer arrayPtr;
        };
        std::vector<AttributeInfo> attrInfos(this->m_codecParams.attrParams.size());

        // 进度控制
        std::atomic<double> progressSum{ 0.0 };
        std::mutex progressMutex;

        ThreadPool::parallelFor(0, this->m_codecParams.attrParams.size(), [&](int start, int end) -> void {
            for (int i = start; i < end; i++) {
                auto& params = this->m_codecParams.attrParams[i];
                int valueCount = params.elementCount * params.dimension;

                // 计算和获取当前属性对应的二进制数据
                int localBinaryCursor;
                std::vector<unsigned char> inputBuffer;
                {
                    std::lock_guard<std::mutex> lock(progressMutex); // 保护共享的binaryCursor
                    localBinaryCursor = binaryCursor;
                    binaryCursor += params.binaryCount;

                    inputBuffer.assign(
                        uCharBuffer.begin() + localBinaryCursor,
                        uCharBuffer.begin() + localBinaryCursor + params.binaryCount
                    );
                }

                auto processType = [&](auto& container, auto valueType) {
                    using ValueType = decltype(valueType);
                    using ArrayType = FlatArray<ValueType>;
                    auto decoded = ArrayType::New();
                    decoded->Resize(valueCount);
                    MeshOptFloatCodec::FloatDecoder(container, inputBuffer, params);
                    memcpy(decoded->RawPointer(), container.data(), valueCount * sizeof(ValueType));
                    decoded->SetDimension(params.dimension);
                    decoded->SetName(params.name);

                    // 保存解码结果而不是立即添加
                    attrInfos[i] = AttributeInfo{ params.type, params.attachmentType, decoded };
                    };

                if (params.valueSize == sizeof(float)) {
                    std::vector<float> floats;
                    processType(floats, float{});
                }
                else if (params.valueSize == sizeof(double)) {
                    std::vector<double> doubles;
                    processType(doubles, double{});
                }

                // 更新进度
                double progressIncrement = 0.2 * (1.0 / this->m_codecParams.attrParams.size());
                progressSum.fetch_add(progressIncrement);

                // 定期更新总进度（减少互斥锁争用）
                if (i % 10 == 0 || i == end - 1) {
                    std::lock_guard<std::mutex> lock(progressMutex);
                    UpdateProgress(m_DecompressProgress + progressSum.load());
                }
            }
            });

        // 更新最终进度
        m_DecompressProgress += progressSum.load();
        UpdateProgress(m_DecompressProgress);

        // 所有线程完成后，按顺序添加属性
        for (const auto& attrInfo : attrInfos) {
            if (attrInfo.arrayPtr) { // 确保指针有效
                this->m_DecoderAdapter->AddAttribute(attrInfo.type, attrInfo.attachmentType, attrInfo.arrayPtr);
            }
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

        while (true)
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

        if (this->m_codecParams.topoParams.isSecondaryIndex)
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
                this->m_codecParams.topoParams.bottomCellBufferBinaryCount,
                this->m_codecParams.topoParams.bottomCellBufferSize,
                this->m_codecParams.topoParams.bottomCellBufferPadding,
                -1,
                this->m_codecParams.topoParams.bottomCellSizeBinaryCount,
                face2pointsSize
            );

            // 解码 体 -> 面
            IndexNOffsetDecoder
            (
                inputTopo,
                inputCursor,
                volume2facesIndex,
                this->m_codecParams.topoParams.topCellBufferBinaryCount,
                this->m_codecParams.topoParams.topCellBufferSize,
                this->m_codecParams.topoParams.topCellBufferPadding,
                -1,
                this->m_codecParams.topoParams.topCellSizeBinaryCount,
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
            int fixedCellSize = this->m_codecParams.topoParams.fixedCellSize;
            bool isFixedCellSize = fixedCellSize != -1;

            if (isFixedCellSize)
            {
                IndexNOffsetDecoder
                (
                    inputTopo,
                    inputCursor,
                    volume2pointsIndex,
                    this->m_codecParams.topoParams.topCellBufferBinaryCount,
                    this->m_codecParams.topoParams.topCellBufferSize,
                    this->m_codecParams.topoParams.topCellBufferPadding,
                    fixedCellSize,
                    this->m_codecParams.topoParams.topCellSizeBinaryCount, // 后两个参数无实际作用
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
                    this->m_codecParams.topoParams.topCellBufferBinaryCount,
                    this->m_codecParams.topoParams.topCellBufferSize,
                    this->m_codecParams.topoParams.topCellBufferPadding,
                    -1,
                    this->m_codecParams.topoParams.topCellSizeBinaryCount,
                    volume2pointsSize
                );
            }

            // 如果是非结构化网格 还要考虑types
            if (this->m_codecParams.meshType == IG_UNSTRUCTURED_MESH)
            {
                this->CellTypeDecoder(
                    outCellTypes,
                    inputTopo.data() + inputCursor,
                    this->m_codecParams.topoParams.cellTypeBinaryCount
                );
                inputCursor += this->m_codecParams.topoParams.cellTypeBinaryCount;
            }

            // 写入网格

            bool unstructuredFlag = this->m_codecParams.meshType == IG_UNSTRUCTURED_MESH;
            bool structuredFlag = this->m_codecParams.meshType == IG_STRUCTURED_MESH;

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
                this->m_DecoderAdapter->AddStructureCells(volume2pointsIndex, this->m_codecParams.structuredMeshParams.axisSize);
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

        m_DecompressProgress += 0.2;
        UpdateProgress(m_DecompressProgress);
    }

    bool OpenStream(std::string path)
    {
        this->m_BytestreamFile.open(path, std::ios::binary);
        if (!this->m_BytestreamFile.is_open()) {
            return false;
        }
        return true;
    }

    void closeStream()
    {
        this->m_BytestreamFile.clear();
        this->m_BytestreamFile.seekg(0, std::ios_base::end);
        //std::cout << "Total bitstream size " << this->m_BytestreamFile.tellg() << " B" << std::endl;
    }
};
IGAME_NAMESPACE_END
#endif