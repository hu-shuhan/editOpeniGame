#ifndef MeshDecoder_h
#define MeshDecoder_h


#include "MeshCodec/DecodeAdapter/iGameMeshDecodeAdapterToDataObject.h"
#include "MeshCodec/DecodeInput/iGameIDecodeInput.h"
#include "MeshCodec/DecodeOutput/iGameIDecodeOutput.h"
#include "MeshCodec/DecodeOutput/iGameDecodeOutputDataObject.h"
#include "MeshCodec/SubCodec/iGameMeshFloatCodec.h"
#include "MeshCodec/SubCodec/iGameMeshIndexCodec.h"
#include "MeshCodec/Utils/iGameMeshCodecParamSet.h"
#include "MeshCodec/Utils/iGameMeshCodecThread.h"
#include "iGameMacro.h"
#include "MeshCodec/iGameMeshCodec.h"
#include "MeshCodec/SubCodec/iGameMeshCodecZSTD.h"

#include <algorithm>
#include <memory>

IGAME_NAMESPACE_BEGIN

// 类型特征：OutputType -> DecodeOutputClass 映射
// 调用方可以提供自己的特化
template<typename OutputType>
struct DecodeOutputTraits;

// DataObject::Pointer 的特化
template<>
struct DecodeOutputTraits<DataObject::Pointer> {
    using OutputClass = DecodeOutputDataObject;
};

template<typename OutputType>
class MeshDecoderFilter final : public MeshCodec {
    using DecodeOutputClass = typename DecodeOutputTraits<OutputType>::OutputClass;
public:
    I_OBJECT(MeshDecoderFilter);
    static Pointer New() { return new MeshDecoderFilter; }

    MeshDecoderFilter() {
        this->SetNumberOfInputs(1);
        this->SetNumberOfOutputs(1);
        m_DecoderOutput = DecodeOutputClass::New();
    }

    bool Execute() override {
        if (!InitializeInputs()) { return false; }

        m_DecompressProgress = 0.0f;

        PayloadBuffer buf;
        while (m_DecoderInput->ReadPayload(&buf)) { ProcessPayload(buf); }

        // 将 adapter 的输出设置到 DecodeOutput
        if (m_DecoderAdapter) {
            m_DecoderOutput->SetOutput(m_DecoderAdapter->GetOutput());
        }

        SetOutput(0, m_DecoderOutput);
        UpdateProgress(1.0);
        return true;
    }

    void SetAdapter(std::unique_ptr<IMeshDecodeAdapter<OutputType>> adapter) {
        m_DecoderAdapter = std::move(adapter);
    }

    IMeshDecodeAdapter<OutputType>* GetAdapter() const {
        return m_DecoderAdapter.get();
    }

    typename DecodeOutputClass::Pointer GetDecodeOutput() const {
        return m_DecoderOutput;
    }

private:
    // I/O
    std::unique_ptr<IMeshDecodeAdapter<OutputType>> m_DecoderAdapter;
    typename DecodeOutputClass::Pointer m_DecoderOutput;
    IDecodeInput::Pointer m_DecoderInput;

    // progress record
    float m_DecompressProgress = 0.0f;

    // region caller
    void ProcessPayload(PayloadBuffer& buf) {
        PayloadType type = buf.type;

        PayloadBuffer bufDecompressed;
        MeshCodecZSTD().Decompress(bufDecompressed, buf);

        switch (type) {
            case PayloadType::kParameterSet: {
                this->ParamsDecoder(bufDecompressed);
                if (m_DecoderAdapter) {
                    m_DecoderAdapter->SetMeshType(this->m_codecParams.meshType);
                }
                break;
            }
            case PayloadType::kGeometryBrick: {
                this->GeomDecoder(bufDecompressed);
                break;
            }
            case PayloadType::kAttributeBrick: {
                this->AttrDecoder(bufDecompressed);
                break;
            }
            case PayloadType::kTopologyBrick: {
                this->TopoDecoder(bufDecompressed);
                break;
            }
            default:
                break;
        }
    }
    // endregion

    // region main decoders
    void ParamsDecoder(PayloadBuffer& buf) {
        IGsize staticSize = sizeof(ParametersWoAttr);

        // 读取静态数据
        ParametersWoAttr paramsWoAttr{};
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

    void GeomDecoder(PayloadBuffer& buf) {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        m_DecompressProgress += 0.05;
        UpdateProgress(m_DecompressProgress);

        IGsize bufferSize = this->m_codecParams.geomParams.elementCount * this->m_codecParams.geomParams.dimension;
        std::vector<float> decodedFloat(bufferSize);

        MeshFloatCodec::FloatDecoder(decodedFloat, uCharBuffer, this->m_codecParams.geomParams);

        m_DecompressProgress += 0.15;
        UpdateProgress(m_DecompressProgress);

        this->m_DecoderAdapter->SetPointBuffer(decodedFloat);
    }

    void AttrDecoder(PayloadBuffer& buf) {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        // Pre-calculate all binary cursor positions
        std::vector<IGsize> binaryCursorOffsets(this->m_codecParams.attrParams.size() + 1);
        binaryCursorOffsets[0] = 0;
        for (size_t i = 0; i < this->m_codecParams.attrParams.size(); i++) {
            binaryCursorOffsets[i + 1] = binaryCursorOffsets[i] + this->m_codecParams.attrParams[i].binaryCount;
        }

        // 用于暂存所有线程产生的属性数据
        std::vector<AttributeBuffer> attrBuffers(this->m_codecParams.attrParams.size());

        // 进度控制
        CodecProgressParallelFor(this, 0,
                            static_cast<int>(this->m_codecParams.attrParams.size()),
                            m_DecompressProgress, m_DecompressProgress + 0.2f,
                            [&](int start, int end) -> void {
            for (int i = start; i < end; i++) {
                auto& params = this->m_codecParams.attrParams[i];

                // 参数有效性检查
                if (params.dimension <= 0 || params.elementCount <= 0) {
                    continue;
                }

                // 使用预先计算的位置获取当前属性的二进制数据
                std::vector<unsigned char> inputBuffer(
                    uCharBuffer.begin() + binaryCursorOffsets[i],
                    uCharBuffer.begin() + binaryCursorOffsets[i + 1]
                );

                // 提取属性名
                const char* nameStart = params.name;
                const char* nameEnd = std::find(nameStart, nameStart + sizeof(params.name), '\0');
                const std::string attributeName(nameStart, nameEnd);

                const IGsize valueCount = params.elementCount * params.dimension;

                // 构建 AttributeBuffer
                AttributeBuffer attr;
                attr.name = attributeName;
                attr.type = params.type;
                attr.attachmentType = params.attachmentType;
                attr.dimension = params.dimension;
                attr.valueSize = params.valueSize;

                if (params.valueSize == sizeof(float)) {
                    std::vector<float> floats;
                    MeshFloatCodec::FloatDecoder(floats, inputBuffer, params);
                    // 解码结果大小验证
                    if (floats.size() != valueCount) {
                        continue;
                    }
                    attr.floatData = std::move(floats);
                } else if (params.valueSize == sizeof(double)) {
                    std::vector<double> doubles;
                    MeshFloatCodec::FloatDecoder(doubles, inputBuffer, params);
                    // 解码结果大小验证
                    if (doubles.size() != valueCount) {
                        continue;
                    }
                    attr.doubleData = std::move(doubles);
                }

                // 保存解码结果
                attrBuffers[i] = std::move(attr);
            }
        });

        // 更新最终进度
        m_DecompressProgress += 0.2;
        UpdateProgress(m_DecompressProgress);

        // 所有线程完成后，按顺序添加属性
        for (const auto& attr : attrBuffers) {
            this->m_DecoderAdapter->AddAttribute(attr);
        }
    }

    void TopoDecoder(PayloadBuffer& buf) {
        if (this->m_codecParams.meshType == IG_POINT_SET) {
            // 点云没有拓扑结构，直接更新进度并返回
            m_DecompressProgress += 0.2;
            UpdateProgress(m_DecompressProgress);
            return;
        }

        // 结构化网格：不需要解码cell连接关系，通过axisSize自动生成
        if (this->m_codecParams.meshType == IG_STRUCTURED_MESH) {
            this->m_DecoderAdapter->SetStructuredMeshDimension(this->m_codecParams.structuredMeshParams.axisSize);

            m_DecompressProgress += 0.2;
            UpdateProgress(m_DecompressProgress);
            return;
        }

        std::vector<unsigned char> inputTopo(buf.size());
        std::memcpy(inputTopo.data(), buf.data(), buf.size());

        int inputCursor = 0;

        if (this->m_codecParams.topoParams.isSecondaryIndex) {
            std::vector<unsigned int> volume2facesIndex;
            std::vector<unsigned int> volume2facesSize;
            std::vector<unsigned int> face2pointsIndex;
            std::vector<unsigned int> face2pointsSize;

            // 解码 面 -> 顶点
            IndexNOffsetDecoder(inputTopo, inputCursor, face2pointsIndex,
                                this->m_codecParams.topoParams.bottomCellBufferBinaryCount,
                                this->m_codecParams.topoParams.bottomCellBufferSize,
                                this->m_codecParams.topoParams.bottomCellBufferPadding,
                                -1,
                                this->m_codecParams.topoParams.bottomCellSizeBinaryCount,
                                face2pointsSize);

            // 解码 体 -> 面
            IndexNOffsetDecoder(inputTopo, inputCursor, volume2facesIndex,
                                this->m_codecParams.topoParams.topCellBufferBinaryCount,
                                this->m_codecParams.topoParams.topCellBufferSize,
                                this->m_codecParams.topoParams.topCellBufferPadding,
                                -1,
                                this->m_codecParams.topoParams.topCellSizeBinaryCount,
                                volume2facesSize);

            this->m_DecoderAdapter->AddSecondaryIndexCells(
                volume2facesIndex,
                volume2facesSize,
                face2pointsIndex,
                face2pointsSize);
        } else {
            std::vector<unsigned int> volume2pointsIndex;
            std::vector<unsigned int> volume2pointsSize;
            std::vector<unsigned int> outCellTypes;
            int fixedCellSize = this->m_codecParams.topoParams.fixedCellSize;
            bool isFixedCellSize = fixedCellSize != -1;

            if (isFixedCellSize) {
                IndexNOffsetDecoder(inputTopo, inputCursor, volume2pointsIndex,
                                    this->m_codecParams.topoParams.topCellBufferBinaryCount,
                                    this->m_codecParams.topoParams.topCellBufferSize,
                                    this->m_codecParams.topoParams.topCellBufferPadding,
                                    fixedCellSize,
                                    // 后两个参数无实际作用
                                    this->m_codecParams.topoParams.topCellSizeBinaryCount,
                                    volume2pointsSize);
            } else {
                IndexNOffsetDecoder(inputTopo, inputCursor, volume2pointsIndex,
                                    this->m_codecParams.topoParams.topCellBufferBinaryCount,
                                    this->m_codecParams.topoParams.topCellBufferSize,
                                    this->m_codecParams.topoParams.topCellBufferPadding,
                                    -1,
                                    this->m_codecParams.topoParams.topCellSizeBinaryCount,
                                    volume2pointsSize);
            }

            // 如果是非结构化网格 还要考虑types
            if (this->m_codecParams.meshType == IG_UNSTRUCTURED_MESH) {
                this->CellTypeDecoder(
                    outCellTypes,
                    inputTopo.data() + inputCursor,
                    this->m_codecParams.topoParams.cellTypeBinaryCount);
                inputCursor += this->m_codecParams.topoParams.cellTypeBinaryCount;
            }

            // 写入网格
            bool unstructuredFlag = this->m_codecParams.meshType == IG_UNSTRUCTURED_MESH;

            // 非结构化网格
            if (unstructuredFlag) {
                if (isFixedCellSize) {
                    this->m_DecoderAdapter->AddUnstructuredFixedCells(
                        volume2pointsIndex,
                        fixedCellSize,
                        outCellTypes);
                } else {
                    this->m_DecoderAdapter->AddUnstructuredPolyCells(
                        volume2pointsIndex,
                        volume2pointsSize,
                         outCellTypes);
                }
            } else {
                // SurfaceMesh或VolumeMesh（非结构化）
                if (isFixedCellSize) {
                    this->m_DecoderAdapter->AddSameTypeFixedCells(
                        volume2pointsIndex, fixedCellSize);
                } else {
                    this->m_DecoderAdapter->AddSameTypePolyCells(
                        volume2pointsIndex, volume2pointsSize);
                }
            }
        }

        m_DecompressProgress += 0.2;
        UpdateProgress(m_DecompressProgress);
    }
    // endregion

    // region I/O
    bool InitializeInputs() {
        m_DecoderInput = DynamicCast<IDecodeInput>(GetInput(0));
        if (!m_DecoderInput) { return false; }
        return m_DecoderInput->Initialize();
    }
    // endregion

    // region sub-decoders
    unsigned int decodeVByte(unsigned char*& data) {
        unsigned char lead = *data++;

        // fast path: single byte
        if (lead < 128) return lead;

        // slow path: up to 4 extra bytes
        // note that this loop always terminates, which is important for malformed data
        unsigned int result = lead & 127;
        unsigned int shift = 7;

        for (int i = 0; i < 4; ++i) {
            unsigned char group = *data++;
            result |= unsigned(group & 127) << shift;
            shift += 7;

            if (group < 128) break;
        }

        return result;
    }

    void RunLengthDecoder(std::vector<uint32_t>& dest, unsigned char* source, int sourceCount) {
        unsigned char* start = source;

        while (true) {
            // 读数据
            uint32_t readInt;
            std::memcpy(&readInt, source, sizeof(uint32_t));
            source += sizeof(uint32_t);

            // 读长度
            unsigned int length = this->decodeVByte(source);

            // 写入
            for (int j = 0; j < length; j++) { dest.push_back(readInt); }

            if (source - start == sourceCount) { break; }
        }

        return;
    }

    void CellBufferDecoder(std::vector<uint32_t>& dest, const unsigned char* source, int sourceCount,
                           int bufferPadding) {
        MeshIndexCodec::decodeIndexBuffer(dest.data(), dest.size(), source, sourceCount);
        dest.resize(dest.size() - bufferPadding);
    }

    void CellTypeDecoder(std::vector<uint32_t>& dest, unsigned char* source, int sourceCount) {
        this->RunLengthDecoder(dest, source, sourceCount);
    }

    void CellSizeDecoder(std::vector<uint32_t>& dest, unsigned char* source, int sourceCount) {
        this->RunLengthDecoder(dest, source, sourceCount);
    }

    void IndexNOffsetDecoder(std::vector<unsigned char>& inputTopo, int& cursor,

                             std::vector<unsigned int>& outIndices, const IGsize indicesBinaryCount,
                             const IGsize indicesBufferSize, const int indicesBufferPadding,

                             int fixedCellSize, const IGsize cellSizeBinaryCount,
                             std::vector<unsigned int>& outCellSizes

    ) {
        // 解码cellbuffer
        outIndices.resize(indicesBufferSize + indicesBufferPadding);
        this->CellBufferDecoder(outIndices, inputTopo.data() + cursor, indicesBinaryCount, indicesBufferPadding);
        cursor += indicesBinaryCount;

        // 解码size
        if (fixedCellSize == -1) {
            this->CellSizeDecoder(outCellSizes, inputTopo.data() + cursor, cellSizeBinaryCount);
            cursor += cellSizeBinaryCount;
        }
    }
    // endregion

    // region deprecated
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
    //             this->m_DecoderAdapter = new MeshDecodeAdapterToDataObject(this->m_codecParams.meshType);
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
    //     return this->m_DecoderAdapter->GetOutput();
    // }
    //
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
    //
    //
    //void AttrDecoder(PayloadBuffer& buf)
    //{
    //    std::vector<unsigned char> uCharBuffer(buf.size());
    //    std::memcpy(uCharBuffer.data(), buf.data(), buf.size());
    //
    //    int binaryCursor = 0;
    //    for (int i = 0; i < this->m_codecParams.attrParams.size(); i++)
    //    {
    //        auto& params = this->m_codecParams.attrParams[i];
    //        int dimension = params.dimension;
    //        int elementCount = params.elementCount;
    //        int valueCount = params.elementCount * params.dimension;
    //
    //        std::vector<unsigned char> inputBuffer(
    //            uCharBuffer.begin() + binaryCursor,
    //            uCharBuffer.begin() + binaryCursor + params.binaryCount
    //        );
    //
    //        binaryCursor += params.binaryCount;
    //
    //        std::vector<float> floats;
    //        std::vector<double> doubles;
    //
    //
    //        auto processType = [&](auto& container, auto valueType) {
    //            using ValueType = decltype(valueType);
    //            using ArrayType = FlatArray<ValueType>;
    //
    //            auto decoded = ArrayType::New();
    //            decoded->Resize(valueCount);
    //
    //            MeshFloatCodec::FloatDecoder(container, inputBuffer, this->m_codecParams.attrParams[i]);
    //            memcpy(decoded->RawPointer(), container.data(), valueCount * sizeof(ValueType));
    //
    //            decoded->SetDimension(params.dimension);
    //            decoded->SetName(params.name);
    //            this->m_DecoderAdapter->AddAttribute(params.type, params.attachmentType, decoded);
    //            };
    //
    //        // 调用部分
    //        if (params.valueSize == sizeof(float)) {
    //            processType(floats, float{});
    //        }
    //        else if (params.valueSize == sizeof(double)) {
    //            processType(doubles, double{});
    //        }
    //
    //        /*
    //        FlatArray<float>::Pointer decodedFloat{ nullptr };
    //        FlatArray<double>::Pointer decodedDouble{ nullptr };
    //
    //        if (params.valueSize == sizeof(float))
    //        {
    //            decodedFloat = FlatArray<float>::New();
    //            decodedFloat->Resize(valueCount);
    //
    //            MeshFloatCodec::FloatDecoder(floats, inputBuffer, this->m_codecParams.attrParams[i]);
    //            memcpy(decodedFloat->RawPointer(), floats.data(), valueCount * sizeof(float));
    //            decodedFloat->SetDimension(params.dimension);
    //            decodedFloat->SetName(params.name);
    //            this->m_DecoderAdapter->AddAttribute(params.type, params.attachmentType, decodedFloat);
    //
    //            //this->FloatDecoder(decodedFloat->RawPointer(), inputBuffer, params);
    //        }
    //        else if (params.valueSize == sizeof(double))
    //        {
    //            decodedDouble = FlatArray<double>::New();
    //            decodedDouble->Resize(valueCount);
    //
    //            MeshFloatCodec::FloatDecoder(doubles, inputBuffer, this->m_codecParams.attrParams[i]);
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
    //endregion
};

IGAME_NAMESPACE_END
#endif
