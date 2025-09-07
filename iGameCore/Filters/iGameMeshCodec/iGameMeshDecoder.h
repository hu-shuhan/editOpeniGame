#ifndef MeshLoomDecoder_h
#define MeshLoomDecoder_h

#include "iGameMacro.h"
#include "iGameMeshCodec.h"
#include "iGameMeshCodecLZMA.h"
#include "iGameMeshCodecParamSet.h"
#include "iGameMeshDecoderAdapter.h"
#include "iGameMeshFloatCodec.h"
#include <future>

IGAME_NAMESPACE_BEGIN

/**
 * @brief MeshEncoder - 网格解码器类
 *
 * Filter Input Number: 1
 * Filter Input:
 *
 * Filter Output Number: 1
 * Filter Output: DataObject(DynamicCastTo: SurfaceMesh, VolumeMesh, StructuredMesh, UnstructuredMesh)
 *
 * Parameters Number:
 * Parameters:
 *
 */

class MeshDecoder : public MeshCodec {
public:
    I_OBJECT(MeshDecoder);
    static Pointer New() { return new MeshDecoder; }

    MeshDecoder()
    {
        this->SetNumberOfInputs(0);
        this->SetNumberOfOutputs(1);
    }

    void SetMemoryMappedPointers(char*& fileStart,
                               const char*& currentPos,
                               char*& fileEnd,
                               const size_t& fileSize) {
        m_FILESTART = fileStart;
        m_IS = currentPos;
        m_FILEEND = fileEnd;
        m_FileSize = fileSize;
    }

    bool Execute() override
    {
        if (!m_FILESTART || !m_FILEEND || m_IS >= m_FILEEND) {
            return false;
        }

        m_DecompressProgress = 0.0f;

        PayloadBuffer buf;
        while (m_IS < m_FILEEND) {
            if (!ReadBufFromMemory(&buf)) {
                break;
            }

            PayloadType type = buf.type;

            PayloadBuffer bufDecompressed;
            MeshCodecLZMA::Decompress(bufDecompressed, buf);

            switch (type)
            {
            case PayloadType::kParameterSet:
            {
                this->ParamsDecoder(bufDecompressed);
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
        }

        UpdateProgress(1.0);

        SetOutput(0, m_DecoderAdapter->GetDataObj());

        return true;
    }

    // 保留原有的 Execute 方法（用于向后兼容）
    // DataObject::Pointer ExecuteWithFilePath()
    // {
    //     if (!m_ReadFilePath.empty() && !this->OpenStream(m_ReadFilePath)) { return nullptr; }
    //
    //     PayloadBuffer buf;
    //     while (true) {
    //         ReadBuf(this->m_BytestreamFile, &buf);
    //         PayloadType type = buf.type;
    //
    //         PayloadBuffer bufDecompressed;
    //         MeshCodecLZMA::Decompress(bufDecompressed, buf);
    //
    //         switch (type)
    //         {
    //         case PayloadType::kParameterSet:
    //         {
    //             // 虽然这里没有控制顺序 但编码时应严格要求首先写入params
    //             this->ParamsDecoder(bufDecompressed);
    //
    //             // 初始adapter
    //             this->m_DecoderAdapter = new MeshDecoderAdapter(this->m_codecParams.meshType);
    //             break;
    //         }
    //         case PayloadType::kGeometryBrick:
    //         {
    //             this->GeomDecoder(bufDecompressed);
    //             break;
    //         }
    //         case PayloadType::kAttributeBrick:
    //         {
    //             this->AttrDecoder(bufDecompressed);
    //             break;
    //         }
    //         case PayloadType::kTopologyBrick:
    //         {
    //             this->TopoDecoder(bufDecompressed);
    //             break;
    //         }
    //         default:
    //             break;
    //         }
    //
    //         //switch (buf.type)
    //         //{
    //         //case PayloadType::kParameterSet:
    //         //{
    //         //    // 虽然这里没有控制顺序 但编码时应严格要求首先写入params
    //         //    this->ParamsDecoder(buf);
    //         //    break;
    //         //}
    //         //case PayloadType::kGeometryBrick:
    //         //{
    //         //    this->GeomDecoder(buf);
    //         //    break;
    //         //}
    //         //case PayloadType::kAttributeBrick:
    //         //{
    //         //    this->AttrDecoder(buf);
    //         //    break;
    //         //}
    //         //case PayloadType::kTopologyBrick:
    //         //{
    //         //    this->TopoDecoder(buf);
    //         //    break;
    //         //}
    //         //default:
    //         //    break;
    //         //}
    //
    //         // at end of file (or other error), flush decoder
    //         if (!this->m_BytestreamFile)
    //         {
    //             break;
    //         }
    //     }
    //
    //     UpdateProgress(1.0);
    //
    //     closeStream();
    //
    //     return this->m_DecoderAdapter->GetDataObj();
    // }

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

    void AttrDecoder(PayloadBuffer& buf)
    {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        // Pre-calculate all binary cursor positions
        std::vector<int> binaryCursorOffsets(this->m_codecParams.attrParams.size() + 1);
        binaryCursorOffsets[0] = 0;
        for (int i = 0; i < this->m_codecParams.attrParams.size(); i++) {
            binaryCursorOffsets[i + 1] = binaryCursorOffsets[i] + this->m_codecParams.attrParams[i].binaryCount;
        }

        // 用于暂存所有线程产生的属性数据
        struct AttributeInfo {
            IGenum type;
            IGenum attachmentType;
            ArrayObject::Pointer arrayPtr;
        };
        std::vector<AttributeInfo> attrInfos(this->m_codecParams.attrParams.size());

        // 进度控制
        ProgressParallelFor(0, this->m_codecParams.attrParams.size(),
            m_DecompressProgress, m_DecompressProgress + 0.2,
            [&](int start, int end) -> void {
                for (int i = start; i < end; i++) {
                    auto& params = this->m_codecParams.attrParams[i];
                    int valueCount = params.elementCount * params.dimension;

                    // 使用预先计算的位置获取当前属性的二进制数据
                    std::vector<unsigned char> inputBuffer(
                        uCharBuffer.begin() + binaryCursorOffsets[i],
                        uCharBuffer.begin() + binaryCursorOffsets[i + 1]
                    );

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
                }
            });

        // 更新最终进度
        m_DecompressProgress += 0.2;
        UpdateProgress(m_DecompressProgress);

        // 所有线程完成后，按顺序添加属性
        for (const auto& attrInfo : attrInfos) {
            if (attrInfo.arrayPtr) { // 确保指针有效
                this->m_DecoderAdapter->AddAttribute(attrInfo.type, attrInfo.attachmentType, attrInfo.arrayPtr);
            }
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

private:
    const char* m_IS = nullptr;
    const char* m_FILESTART = nullptr;
    const char* m_FILEEND = nullptr;
    size_t m_FileSize = 0;
    MeshDecoderAdapter* m_DecoderAdapter;
    float m_DecompressProgress = 0.0f;

    bool ReadBufFromMemory(PayloadBuffer* buf) {
        buf->resize(0);
        
        // 存储当前m_IS位置的8个字节内容到变量
        unsigned char debug_bytes[8];
        for(int i = 0; i < 8 && m_IS + i < m_FILEEND; i++) {
            debug_bytes[i] = (unsigned char)m_IS[i];
        }
        
        buf->type = static_cast<PayloadType>(*m_IS++);

        uint32_t length = 0;
        length = (length << 8) | static_cast<unsigned char>(*m_IS++);
        length = (length << 8) | static_cast<unsigned char>(*m_IS++);
        length = (length << 8) | static_cast<unsigned char>(*m_IS++);
        length = (length << 8) | static_cast<unsigned char>(*m_IS++);

        if (m_IS + length > m_FILEEND) {
            return false;
        }

        buf->resize(length);
        std::memcpy(buf->data(), m_IS, length);
        m_IS += length;

        return true;
    }

    // bool ReadBufFromMemory(PayloadBuffer* buf) {
    //     if (m_IS + 5 > m_FILEEND) {
    //         return false;
    //     }
    //
    //     buf->type = static_cast<PayloadType>(*m_IS++);
    //
    //     uint32_t length = 0;
    //     length |= static_cast<uint32_t>(*m_IS++) << 24;
    //     length |= static_cast<uint32_t>(*m_IS++) << 16;
    //     length |= static_cast<uint32_t>(*m_IS++) << 8;
    //     length |= static_cast<uint32_t>(*m_IS++);
    //
    //     if (m_IS + length > m_FILEEND) {
    //         return false;
    //     }
    //
    //     buf->resize(length);
    //     if (length > 0) {
    //         std::memcpy(buf->data(), m_IS, length);
    //         m_IS += length;
    //     }
    //
    //     return true;
    // }


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
};
IGAME_NAMESPACE_END
#endif