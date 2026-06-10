#ifndef MeshDecoder_h
#define MeshDecoder_h


#include "MeshCodec/Archive/iGameCodecBinaryInputArchive.h"
#include "MeshCodec/DecodeAdapter/iGameMeshDecodeAdapterToDataObject.h"
#include "MeshCodec/DecodeInput/iGameIDecodeInput.h"
#include "MeshCodec/DecodeOutput/iGameIDecodeOutput.h"
#include "MeshCodec/SubCodec/iGameMeshCodecZSTD.h"
#include "MeshCodec/SubCodec/iGameMeshFloatCodec.h"
#include "MeshCodec/SubCodec/iGameMeshIndexCodec.h"
#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "MeshCodec/Utils/iGameMeshCodecThread.h"
#include "MeshCodec/iGameMeshCodec.h"
#include "iGameMacro.h"

#include <algorithm>
#include <cstdint>
#include <memory>

IGAME_NAMESPACE_BEGIN

template<typename DecodeOutputType>
class MeshDecoderFilter final : public MeshCodec {
    using OutputType = DecodeOutputType::ValueType;
public:
    I_OBJECT(MeshDecoderFilter);
    static Pointer New() { return new MeshDecoderFilter; }
    static constexpr uint32_t SupportedParamVersion = CodecStorageParamVersion;

    MeshDecoderFilter() {
        this->SetNumberOfInputs(1);
        this->SetNumberOfOutputs(1);
        m_DecoderOutput = DecodeOutputType::New();
    }

    bool Execute() override {
        IGAME_CORE_INFO("[IGC][MeshDecoder] Execute begin");
        // 无论解码成功/失败，都清理进度文本，避免 UI 文案残留
        struct ProgressTextResetGuard {
            ProgressObserver* observer{};
            ~ProgressTextResetGuard() noexcept {
                if (!observer) { return; }
                observer->UpdateText("");
            }
        } resetTextGuard{m_ProgressObserver};

        if (!InitializeInputs()) {
            IGAME_CORE_ERROR("[IGC][MeshDecoder] InitializeInputs failed");
            return false;
        }

        m_DecompressProgress = 0.0f;

        PayloadBuffer buf;
        size_t payloadCount = 0;
        while (m_DecoderInput->ReadPayload(&buf)) {
            ++payloadCount;
            IGAME_CORE_INFO("[IGC][MeshDecoder] Payload #{} type={} compressedBytes={}", payloadCount,
                            PayloadTypeName(buf.type), buf.size());
            if (!ProcessPayload(buf)) {
                IGAME_CORE_ERROR("[IGC][MeshDecoder] ProcessPayload failed at #{} type={}", payloadCount,
                                 PayloadTypeName(buf.type));
                return false;
            }
        }
        IGAME_CORE_INFO("[IGC][MeshDecoder] Payload loop end count={}", payloadCount);

        // 将 adapter 的输出设置到 DecodeOutput
        if (m_DecoderAdapter) {
            m_DecoderOutput->SetOutput(m_DecoderAdapter->GetOutput());
        }
        IGAME_CORE_INFO("[IGC][MeshDecoder] Adapter output={}",
                        m_DecoderOutput->GetOutput() != nullptr ? "valid" : "null");

        SetOutput(0, m_DecoderOutput);
        UpdateProgress(1.0);

        // 解码结束后复位进度条，避免停留在 100%
        // 注意：Filter::UpdateProgress 会受 m_ProgressShift/m_ProgressScale 影响，不能用 UpdateProgress(0.0) 作为“清零”
        m_Progress = 0.0;
        m_ProgressShift = 0.0;
        m_ProgressScale = 1.0;
        if (m_ProgressObserver) {
            m_ProgressObserver->UpdateProgress(0.0);
            m_ProgressObserver->UpdateText("");
        }
        IGAME_CORE_INFO("[IGC][MeshDecoder] Execute success");
        return true;
    }

    void SetAdapter(std::unique_ptr<IMeshDecodeAdapter<OutputType>> adapter) {
        m_DecoderAdapter = std::move(adapter);
    }

private:
    // I/O
    std::unique_ptr<IMeshDecodeAdapter<OutputType>> m_DecoderAdapter;
    typename DecodeOutputType::Pointer m_DecoderOutput;
    IDecodeInput::Pointer m_DecoderInput;

    // progress record
    float m_DecompressProgress = 0.0f;

    const char* PayloadTypeName(PayloadType type) const {
        switch (type) {
            case PayloadType::kParameterSet:
                return "ParameterSet";
            case PayloadType::kGeometryBrick:
                return "GeometryBrick";
            case PayloadType::kAttributeBrick:
                return "AttributeBrick";
            case PayloadType::kTopologyBrick:
                return "TopologyBrick";
            case PayloadType::kCompressedBrick:
                return "CompressedBrick";
            default:
                return "Unknown";
        }
    }

    bool ValidateCodecParams() const {
        switch (this->m_codecParams.meshType) {
            case IG_POINT_SET:
            case IG_SURFACE_MESH:
            case IG_VOLUME_MESH:
            case IG_STRUCTURED_MESH:
            case IG_UNSTRUCTURED_MESH:
                break;
            default:
                return false;
        }

        if (this->m_codecParams.geomParams.valueSize != sizeof(float)) { return false; }
        if (this->m_codecParams.geomParams.dimension != 3) { return false; }

        for (const auto& attr : this->m_codecParams.attrParams) {
            if (attr.dimension <= 0) { return false; }
            if (attr.valueSize != sizeof(float) && attr.valueSize != sizeof(double)) { return false; }
            if (attr.attachmentType != IG_POINT && attr.attachmentType != IG_CELL) { return false; }
        }

        return true;
    }

    // region caller
    bool ProcessPayload(PayloadBuffer& buf) {
        PayloadType type = buf.type;

        PayloadBuffer bufDecompressed;
        if (!MeshCodecZSTD().Decompress(bufDecompressed, buf)) {
            IGAME_CORE_ERROR("[IGC][MeshDecoder] ZSTD decompress failed type={} compressedBytes={}",
                             PayloadTypeName(type), buf.size());
            return false;
        }
        IGAME_CORE_INFO("[IGC][MeshDecoder] ZSTD decompress ok type={} compressedBytes={} rawBytes={}",
                        PayloadTypeName(type), buf.size(), bufDecompressed.size());

        switch (type) {
            case PayloadType::kParameterSet: {
                if (!this->ParamsDecoder(bufDecompressed)) { return false; }
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
                if (!this->AttrDecoder(bufDecompressed)) { return false; }
                break;
            }
            case PayloadType::kTopologyBrick: {
                this->TopoDecoder(bufDecompressed);
                break;
            }
            default:
                IGAME_CORE_WARN("[IGC][MeshDecoder] Skip unknown payload type={}", static_cast<int>(type));
                break;
        }
        return true;
    }
    // endregion

    // region main decoders
    bool ParamsDecoder(PayloadBuffer& buf) {
        IGAME_CORE_INFO("[IGC][MeshDecoder] ParamsDecoder begin rawBytes={} headerBytes={}", buf.size(),
                        sizeof(CodecStorageHeader));
        if (buf.size() < sizeof(CodecStorageHeader)) {
            IGAME_CORE_ERROR("Invalid IGC parameter payload");
            return false;
        }

        CodecStorageHeader header{};
        std::memcpy(&header, buf.data(), sizeof(CodecStorageHeader));
        IGAME_CORE_INFO("[IGC][MeshDecoder] ParamsDecoder header version={} attrUseCrossDependency={}",
                        header.version, header.attrUseCrossDependency ? "true" : "false");
        if (header.version != SupportedParamVersion) {
            IGAME_CORE_ERROR("Unsupported IGC parameter version {}. Please update or regenerate this file", header.version);
            return false;
        }

        m_DecompressProgress += 0.1;
        UpdateProgress(m_DecompressProgress);

        std::vector<uint8_t> data(buf.size() - sizeof(CodecStorageHeader));
        if (!data.empty()) {
            std::memcpy(data.data(), buf.data() + sizeof(CodecStorageHeader), data.size());
        }
        CodecBinaryInputArchive ar(data);
        this->m_codecParams.Archive(ar);
        IGAME_CORE_INFO("[IGC][MeshDecoder] ParamsDecoder archive cursor={} dataBytes={}", ar.GetCursor(),
                        data.size());
        if (ar.HasError()) {
            IGAME_CORE_ERROR("[IGC][MeshDecoder] ParamsDecoder archive reported out-of-range read");
            return false;
        }
        if (ar.GetCursor() != data.size()) {
            IGAME_CORE_WARN("[IGC][MeshDecoder] ParamsDecoder archive did not consume all bytes cursor={} dataBytes={}",
                            ar.GetCursor(), data.size());
        }
        IGAME_CORE_INFO("[IGC][MeshDecoder] Parsed params before validate: meshType={} geomValueSize={} geomCount={} "
                "geomDim={} attrCount={} topoFixed={} topBufferBytes={} topSizeBytes={} cellTypeBytes={}",
                this->m_codecParams.meshType,
                this->m_codecParams.geomParams.valueSize,
                this->m_codecParams.geomParams.elementCount,
                this->m_codecParams.geomParams.dimension,
                this->m_codecParams.attrParams.size(),
                this->m_codecParams.topoParams.fixedCellSize,
                this->m_codecParams.topoParams.topCellBufferBinaryCount,
                this->m_codecParams.topoParams.topCellSizeBinaryCount,
                this->m_codecParams.topoParams.cellTypeBinaryCount);
        if (!ValidateCodecParams()) {
            IGAME_CORE_ERROR("Invalid IGC parameter payload");
            return false;
        }

        IGsize attrBinaryBytes = 0;
        for (const auto& attr : this->m_codecParams.attrParams) { attrBinaryBytes += attr.binaryCount; }
        IGAME_CORE_INFO("[IGC][MeshDecoder] ParamsDecoder ok meshType={} geomCount={} geomDim={} topoFixed={} "
                        "topBufferBytes={} topSizeBytes={} cellTypeBytes={} attrs={} attrBinaryBytes={}",
                        this->m_codecParams.meshType, this->m_codecParams.geomParams.elementCount,
                        this->m_codecParams.geomParams.dimension, this->m_codecParams.topoParams.fixedCellSize,
                        this->m_codecParams.topoParams.topCellBufferBinaryCount,
                        this->m_codecParams.topoParams.topCellSizeBinaryCount,
                        this->m_codecParams.topoParams.cellTypeBinaryCount,
                        this->m_codecParams.attrParams.size(), attrBinaryBytes);
        for (size_t i = 0; i < this->m_codecParams.attrParams.size(); ++i) {
            const auto& attr = this->m_codecParams.attrParams[i];
            IGAME_CORE_INFO("[IGC][MeshDecoder] AttrParam #{} name={} attachment={} dim={} elements={} valueSize={} "
                            "binaryBytes={}",
                            i, attr.name, attr.attachmentType, attr.dimension, attr.elementCount, attr.valueSize,
                            attr.binaryCount);
        }

        m_DecompressProgress += 0.1;
        UpdateProgress(m_DecompressProgress);
        return true;
    }

    void GeomDecoder(PayloadBuffer& buf) {
        IGAME_CORE_INFO("[IGC][MeshDecoder] GeomDecoder begin rawBytes={}", buf.size());
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
        IGAME_CORE_INFO("[IGC][MeshDecoder] GeomDecoder end decodedFloats={} points={}", decodedFloat.size(),
                        this->m_codecParams.geomParams.elementCount);
    }

    bool AttrDecoder(PayloadBuffer& buf) {
        IGAME_CORE_INFO("[IGC][MeshDecoder] AttrDecoder begin rawBytes={} attrs={}", buf.size(),
                        this->m_codecParams.attrParams.size());
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        // Pre-calculate all binary cursor positions
        std::vector<IGsize> binaryCursorOffsets(this->m_codecParams.attrParams.size() + 1);
        binaryCursorOffsets[0] = 0;
        for (size_t i = 0; i < this->m_codecParams.attrParams.size(); i++) {
            binaryCursorOffsets[i + 1] = binaryCursorOffsets[i] + this->m_codecParams.attrParams[i].binaryCount;
        }
        IGAME_CORE_INFO("[IGC][MeshDecoder] AttrDecoder binaryCursorEnd={} rawBytes={}",
                        binaryCursorOffsets.back(), uCharBuffer.size());

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
                const IGsize valueCount = params.elementCount * params.dimension;

                // 构建 AttributeBuffer
                AttributeBuffer attr;
                attr.name = params.name;
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
        IGAME_CORE_INFO("[IGC][MeshDecoder] AttrDecoder end addedAttrs={}", attrBuffers.size());
        return true;
    }

    void TopoDecoder(PayloadBuffer& buf) {
        IGAME_CORE_INFO("[IGC][MeshDecoder] TopoDecoder begin rawBytes={} meshType={}", buf.size(),
                        this->m_codecParams.meshType);
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
            IGAME_CORE_INFO("[IGC][MeshDecoder] TopoDecoder secondary end topIndex={} topSize={} bottomIndex={} "
                            "bottomSize={} cursor={}/{}",
                            volume2facesIndex.size(), volume2facesSize.size(), face2pointsIndex.size(),
                            face2pointsSize.size(), inputCursor, inputTopo.size());
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
            IGAME_CORE_INFO("[IGC][MeshDecoder] TopoDecoder single end cellIndex={} cellSize={} cellTypes={} "
                            "cursor={}/{} fixedCellSize={}",
                            volume2pointsIndex.size(), volume2pointsSize.size(), outCellTypes.size(), inputCursor,
                            inputTopo.size(), fixedCellSize);
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
