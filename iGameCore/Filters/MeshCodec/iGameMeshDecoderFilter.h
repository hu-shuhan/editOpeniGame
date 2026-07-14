#ifndef MeshDecoder_h
#define MeshDecoder_h


#include "MeshCodec/Archive/iGameCodecBinaryInputArchive.h"
#include "MeshCodec/Archive/iGameCodecLegacyV1Probe.h"
#include "MeshCodec/DecodeAdapter/iGameMeshDecodeAdapterToDataObject.h"
#include "MeshCodec/DecodeInput/iGameIDecodeInput.h"
#include "MeshCodec/DecodeOutput/iGameIDecodeOutput.h"
#include "MeshCodec/SubCodec/iGameMeshCodecZSTD.h"
#include "MeshCodec/SubCodec/iGameMeshFloatCodec.h"
#include "MeshCodec/SubCodec/iGameMeshIndexCodec.h"
#include "MeshCodec/SubCodec/iGameMeshIntegerCodec.h"
#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "MeshCodec/Utils/iGameMeshCodecThread.h"
#include "MeshCodec/iGameMeshCodec.h"
#include "iGameMacro.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>

IGAME_NAMESPACE_BEGIN

template<typename DecodeOutputType>
class MeshDecoderFilter final : public MeshCodec {
    using OutputType = DecodeOutputType::ValueType;
public:
    I_OBJECT(MeshDecoderFilter);
    static Pointer New() { return new MeshDecoderFilter; }
    static constexpr uint32_t SupportedParamVersion = 3;
    static constexpr uint32_t LegacyParamVersion = 1;

    MeshDecoderFilter() {
        this->SetNumberOfInputs(1);
        this->SetNumberOfOutputs(1);
        m_DecoderOutput = DecodeOutputType::New();
    }

    bool Execute() override {
        ReleaseRunState();

        // 无论解码成功/失败，都清理进度文本，避免 UI 文案残留
        struct ProgressTextResetGuard {
            ProgressObserver* observer{};
            ~ProgressTextResetGuard() noexcept {
                if (!observer) { return; }
                observer->UpdateText("");
            }
        } resetTextGuard{m_ProgressObserver};

        struct RunStateResetGuard {
            MeshDecoderFilter* self{};
            bool success = false;
            ~RunStateResetGuard() noexcept {
                if (!success && self) { self->ReleaseRunState(); }
            }
        } runStateGuard{this};

        if (!InitializeInputs()) { return false; }

        m_DecompressProgress = 0.0f;

        PayloadBuffer buf;
        while (m_DecoderInput->ReadPayload(&buf)) {
            if (!ProcessPayload(buf)) { return false; }
        }

        // 将 adapter 的输出设置到 DecodeOutput
        if (m_DecoderAdapter) {
            m_DecoderOutput->SetOutput(m_DecoderAdapter->GetOutput());
        }

        SetOutput(0, m_DecoderOutput);
        ReleaseTransientRunState();
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
        runStateGuard.success = true;
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
    uint32_t m_ParamVersion = 0;

    void ReleaseRunState() {
        SetOutput(0, nullptr);
        m_DecoderInput = nullptr;
        m_codecParams = CodecStorageParams{};
        m_ParamVersion = 0;
        if (m_DecoderOutput) { m_DecoderOutput->SetOutput(OutputType{}); }
        if (m_DecoderAdapter) { m_DecoderAdapter->ResetOutput(); }
    }

    void ReleaseTransientRunState() {
        m_DecoderInput = nullptr;
        m_codecParams = CodecStorageParams{};
        m_ParamVersion = 0;
        if (m_DecoderAdapter) { m_DecoderAdapter->ResetOutput(); }
    }

    static bool MulWillOverflow(IGsize a, IGsize b) {
        return b != 0 && a > std::numeric_limits<IGsize>::max() / b;
    }

    static bool AddWillOverflow(IGsize a, IGsize b) {
        return b > std::numeric_limits<IGsize>::max() - a;
    }

    static bool IsSupportedAttributeType(IGenum type) {
        return type >= IG_SCALAR && type < IG_ATTRIBUTE_COUNT;
    }

    static bool IsSupportedAttributeAttachment(IGenum attachmentType) {
        return attachmentType == IG_POINT || attachmentType == IG_CELL;
    }

    static bool IsFloatAttributeArray(IGenum arrayType) {
        return arrayType == IG_FloatArray || arrayType == IG_DoubleArray;
    }

    static bool IsSupportedIntegerAttributeArray(IGenum arrayType) {
        switch (arrayType) {
            case IG_CharArray:
            case IG_UnsignedCharArray:
            case IG_ShortArray:
            case IG_UnsignedShortArray:
            case IG_IntArray:
            case IG_UnsignedIntArray:
            case IG_LongLongArray:
            case IG_UnsignedLongLongArray:
                return true;
            default:
                return false;
        }
    }

    static bool IsSignedIntegerAttributeArray(IGenum arrayType) {
        switch (arrayType) {
            case IG_CharArray:
                return std::numeric_limits<char>::is_signed;
            case IG_ShortArray:
            case IG_IntArray:
            case IG_LongLongArray:
                return true;
            default:
                return false;
        }
    }

    static uint8_t IntegerBitWidth(IGenum arrayType) {
        switch (arrayType) {
            case IG_CharArray:
                return static_cast<uint8_t>(sizeof(char) * 8u);
            case IG_UnsignedCharArray:
                return static_cast<uint8_t>(sizeof(unsigned char) * 8u);
            case IG_ShortArray:
                return static_cast<uint8_t>(sizeof(short) * 8u);
            case IG_UnsignedShortArray:
                return static_cast<uint8_t>(sizeof(unsigned short) * 8u);
            case IG_IntArray:
                return static_cast<uint8_t>(sizeof(int) * 8u);
            case IG_UnsignedIntArray:
                return static_cast<uint8_t>(sizeof(unsigned int) * 8u);
            case IG_LongLongArray:
                return static_cast<uint8_t>(sizeof(long long) * 8u);
            case IG_UnsignedLongLongArray:
                return static_cast<uint8_t>(sizeof(unsigned long long) * 8u);
            default:
                return 0;
        }
    }

    static bool SumComponentBinaryCounts(const AttrStorageParams& attr, IGsize& sum) {
        sum = 0;
        for (const IGsize count : attr.componentBinaryCounts) {
            if (AddWillOverflow(sum, count)) { return false; }
            sum += count;
        }
        return true;
    }

    bool ValidateLegacyAttrParams(const AttrStorageParams& attr, IGsize& attrBinaryTotal) const {
        if (attr.dimension <= 0) { return false; }
        if (attr.valueSize == 0) { return false; }
        if (!IsSupportedAttributeType(attr.type)) { return false; }
        if (attr.attachmentType < IG_POINT || attr.attachmentType > IG_MID_POINT) { return false; }
        if (MulWillOverflow(attr.elementCount, static_cast<IGsize>(attr.dimension))) { return false; }
        if (AddWillOverflow(attrBinaryTotal, attr.binaryCount)) { return false; }
        attrBinaryTotal += attr.binaryCount;
        return true;
    }

    bool ValidateV3AttrParams(const AttrStorageParams& attr, IGsize& attrBinaryTotal) const {
        if (attr.dimension <= 0) { return false; }
        if (!IsSupportedAttributeType(attr.type)) { return false; }
        if (!IsSupportedAttributeAttachment(attr.attachmentType)) { return false; }
        if (MulWillOverflow(attr.elementCount, static_cast<IGsize>(attr.dimension))) { return false; }
        if (AddWillOverflow(attrBinaryTotal, attr.binaryCount)) { return false; }
        attrBinaryTotal += attr.binaryCount;

        if (attr.attrCodec == AttrCodec::FloatMeshopt) {
            if (attr.attrLayout != AttrLayout::ComponentSeries) { return true; }
            if (!IsFloatAttributeArray(attr.arrayType)) { return true; }
            if (attr.arrayType == IG_FloatArray && attr.valueSize != sizeof(float)) { return false; }
            if (attr.arrayType == IG_DoubleArray && attr.valueSize != sizeof(double)) { return false; }
            if (attr.componentBinaryCounts.size() != static_cast<size_t>(attr.dimension)) { return false; }
            IGsize componentTotal = 0;
            if (!SumComponentBinaryCounts(attr, componentTotal)) { return false; }
            return componentTotal == attr.binaryCount;
        }

        if (attr.attrCodec == AttrCodec::IntegerDeltaRleVarint) {
            if (attr.attrLayout != AttrLayout::ComponentSeries) { return true; }
            if (!IsSupportedIntegerAttributeArray(attr.arrayType)) { return true; }
            const uint8_t bitWidth = IntegerBitWidth(attr.arrayType);
            if (bitWidth == 0 || attr.integerBitWidth != bitWidth) { return false; }
            if (attr.valueSize != static_cast<IGsize>(bitWidth / 8u)) { return false; }
            if (attr.integerSigned != IsSignedIntegerAttributeArray(attr.arrayType)) { return false; }
            return true;
        }

        return true;
    }

    void NormalizeLegacyAttrParams() {
        for (auto& attr : this->m_codecParams.attrParams) {
            attr.componentBinaryCounts.clear();
            attr.integerBitWidth = 0;
            attr.integerSigned = false;
            attr.attrLayout = AttrLayout::InterleavedRecord;
            if (attr.valueSize == sizeof(float)) {
                attr.arrayType = IG_FloatArray;
                attr.attrCodec = AttrCodec::FloatMeshopt;
            } else if (attr.valueSize == sizeof(double)) {
                attr.arrayType = IG_DoubleArray;
                attr.attrCodec = AttrCodec::FloatMeshopt;
            } else {
                attr.arrayType = IG_ARRAY_OBJECT;
                attr.attrCodec = AttrCodec::Unsupported;
            }
        }
    }

    bool ValidateStructuredMeshParams() const {
        if (this->m_codecParams.meshType != IG_STRUCTURED_MESH) {
            return true;
        }

        const int* axisSize = this->m_codecParams.structuredMeshParams.axisSize;
        if (axisSize[0] <= 0 || axisSize[1] <= 0 || axisSize[2] < 0) {
            return false;
        }

        const IGsize axisX = static_cast<IGsize>(axisSize[0]);
        const IGsize axisY = static_cast<IGsize>(axisSize[1]);
        const IGsize axisZ = static_cast<IGsize>(axisSize[2] <= 1 ? 1 : axisSize[2]);
        if (MulWillOverflow(axisX, axisY)) {
            return false;
        }
        const IGsize xy = axisX * axisY;
        if (MulWillOverflow(xy, axisZ)) {
            return false;
        }
        return xy * axisZ == this->m_codecParams.geomParams.elementCount;
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
        if (MulWillOverflow(this->m_codecParams.geomParams.elementCount,
                            static_cast<IGsize>(this->m_codecParams.geomParams.dimension))) {
            return false;
        }
        if constexpr (sizeof(std::size_t) < sizeof(uint64_t)) {
            if (CodecStorageParamSizeLimits::ParamsExceed32Bit(this->m_codecParams)) { return false; }
        }
        if (!ValidateStructuredMeshParams()) { return false; }

        IGsize attrBinaryTotal = 0;
        for (const auto& attr : this->m_codecParams.attrParams) {
            if (m_ParamVersion == LegacyParamVersion) {
                if (!ValidateLegacyAttrParams(attr, attrBinaryTotal)) { return false; }
            } else {
                if (!ValidateV3AttrParams(attr, attrBinaryTotal)) { return false; }
            }
        }

        return true;
    }

    // region caller
    bool ProcessPayload(PayloadBuffer& buf) {
        PayloadType type = buf.type;

        PayloadBuffer bufDecompressed;
        if (buf.empty()) {
            // 兼容旧文件中的空可选载荷
            if (!CanAcceptEmptyPayload(type)) {
                return false;
            }
        } else if (!MeshCodecZSTD().Decompress(bufDecompressed, buf)) {
            return false;
        }

        switch (type) {
            case PayloadType::kParameterSet: {
                if (!this->ParamsDecoder(bufDecompressed)) { return false; }
                ApplyDecodedMeshParams();
                break;
            }
            case PayloadType::kGeometryBrick: {
                if (!this->GeomDecoder(bufDecompressed)) { return false; }
                break;
            }
            case PayloadType::kAttributeBrick: {
                if (!this->AttrDecoder(bufDecompressed)) { return false; }
                break;
            }
            case PayloadType::kTopologyBrick: {
                if (!this->TopoDecoder(bufDecompressed)) { return false; }
                break;
            }
            default:
                break;
        }
        return true;
    }

    void ApplyDecodedMeshParams() {
        if (!m_DecoderAdapter) {
            return;
        }
        m_DecoderAdapter->SetMeshType(this->m_codecParams.meshType);
        if (this->m_codecParams.meshType == IG_STRUCTURED_MESH) {
            m_DecoderAdapter->SetStructuredMeshDimension(
                this->m_codecParams.structuredMeshParams.axisSize);
        }
    }

    bool CanAcceptEmptyPayload(PayloadType type) const {
        if (type == PayloadType::kTopologyBrick) {
            return this->m_codecParams.meshType == IG_POINT_SET ||
                   this->m_codecParams.meshType == IG_STRUCTURED_MESH;
        }
        if (type == PayloadType::kAttributeBrick) {
            return this->m_codecParams.attrParams.empty();
        }
        return false;
    }
    // endregion

    // region main decoders
    bool ParamsDecoder(PayloadBuffer& buf) {
        constexpr size_t kCodecStorageHeaderBinarySize = sizeof(uint32_t) + sizeof(uint8_t) + 3 * sizeof(uint8_t);
        if (buf.size() < kCodecStorageHeaderBinarySize) {
            IGAME_CORE_ERROR("Invalid IGC parameter payload");
            return false;
        }

        CodecStorageHeader header{};
        std::vector<uint8_t> headerData(kCodecStorageHeaderBinarySize);
        std::memcpy(headerData.data(), buf.data(), headerData.size());

        try {
            CodecBinaryInputArchive headerAr(headerData);
            header.Archive(headerAr);
        } catch (const std::exception& e) {
            IGAME_CORE_ERROR("Invalid IGC parameter header: {}", e.what());
            return false;
        }

        if (header.version == 2) {
            IGAME_CORE_ERROR("IGC parameter version 2 is not supported. Please regenerate this file as version 3");
            return false;
        }

        if (header.version != SupportedParamVersion && header.version != LegacyParamVersion) {
            IGAME_CORE_ERROR("Unsupported IGC parameter version {}. Please update or regenerate this file", header.version);
            return false;
        }

        m_ParamVersion = header.version;

        if constexpr (sizeof(std::size_t) < sizeof(uint64_t)) {
            if (header.Requires64BitSize()) {
                IGAME_CORE_ERROR("IGC parameter payload requires a 64-bit platform");
                return false;
            }
        }

        m_DecompressProgress += 0.1;
        UpdateProgress(m_DecompressProgress);

        std::vector<uint8_t> data(buf.size() - kCodecStorageHeaderBinarySize);
        if (!data.empty()) {
            std::memcpy(data.data(), buf.data() + kCodecStorageHeaderBinarySize, data.size());
        }

        if (header.version == LegacyParamVersion) {
            IGAME_CORE_WARN("Reading legacy IGC parameter version 1. Please prefer version 3 IGC data when possible");
            std::string error;
            if (!CodecLegacyV1Probe::Decode(data, this->m_codecParams, &error)) {
                IGAME_CORE_ERROR("Invalid legacy IGC parameter payload: {}", error);
                return false;
            }
            NormalizeLegacyAttrParams();
        } else {
            try {
                CodecBinaryInputArchive ar(data);
                this->m_codecParams.Archive(ar);
            } catch (const std::exception& e) {
                IGAME_CORE_ERROR("Invalid IGC parameter payload: {}", e.what());
                return false;
            }
        }

        if (!ValidateCodecParams()) {
            IGAME_CORE_ERROR("Invalid IGC parameter payload");
            return false;
        }

        m_DecompressProgress += 0.1;
        UpdateProgress(m_DecompressProgress);
        return true;
    }

    bool GeomDecoder(PayloadBuffer& buf) {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        m_DecompressProgress += 0.05;
        UpdateProgress(m_DecompressProgress);

        IGsize bufferSize = this->m_codecParams.geomParams.elementCount * this->m_codecParams.geomParams.dimension;
        std::vector<float> decodedFloat(bufferSize);

        MeshFloatCodec::FloatDecoder(decodedFloat, uCharBuffer, this->m_codecParams.geomParams);
        if (decodedFloat.size() != bufferSize) {
            IGAME_CORE_ERROR("Invalid IGC geometry payload");
            return false;
        }

        m_DecompressProgress += 0.15;
        UpdateProgress(m_DecompressProgress);

        this->m_DecoderAdapter->SetPointBuffer(decodedFloat);
        return true;
    }

    template<typename ValueType>
    static bool DecodeIntegerAttribute(
            AttributeBuffer& attr,
            const std::vector<unsigned char>& inputBuffer,
            const AttrStorageParams& params) {
        std::vector<ValueType> values;
        if (!MeshIntegerCodec::Decode<ValueType>(values, inputBuffer, params.elementCount, params.dimension)) {
            return false;
        }

        attr.rawData.resize(values.size() * sizeof(ValueType));
        if (!values.empty()) {
            std::memcpy(attr.rawData.data(), values.data(), attr.rawData.size());
        }
        return true;
    }

    static bool DecodeIntegerAttributeByType(
            AttributeBuffer& attr,
            const std::vector<unsigned char>& inputBuffer,
            const AttrStorageParams& params) {
        switch (params.arrayType) {
            case IG_CharArray:
                return DecodeIntegerAttribute<char>(attr, inputBuffer, params);
            case IG_UnsignedCharArray:
                return DecodeIntegerAttribute<unsigned char>(attr, inputBuffer, params);
            case IG_ShortArray:
                return DecodeIntegerAttribute<short>(attr, inputBuffer, params);
            case IG_UnsignedShortArray:
                return DecodeIntegerAttribute<unsigned short>(attr, inputBuffer, params);
            case IG_IntArray:
                return DecodeIntegerAttribute<int>(attr, inputBuffer, params);
            case IG_UnsignedIntArray:
                return DecodeIntegerAttribute<unsigned int>(attr, inputBuffer, params);
            case IG_LongLongArray:
                return DecodeIntegerAttribute<long long>(attr, inputBuffer, params);
            case IG_UnsignedLongLongArray:
                return DecodeIntegerAttribute<unsigned long long>(attr, inputBuffer, params);
            default:
                return false;
        }
    }

    bool AttrDecoder(PayloadBuffer& buf) {
        std::vector<unsigned char> uCharBuffer(buf.size());
        std::memcpy(uCharBuffer.data(), buf.data(), buf.size());

        // Pre-calculate all binary cursor positions
        std::vector<IGsize> binaryCursorOffsets(this->m_codecParams.attrParams.size() + 1);
        binaryCursorOffsets[0] = 0;
        for (size_t i = 0; i < this->m_codecParams.attrParams.size(); i++) {
            if (this->m_codecParams.attrParams[i].binaryCount >
                static_cast<IGsize>(std::numeric_limits<size_t>::max())) {
                IGAME_CORE_ERROR("Invalid IGC attribute payload");
                return false;
            }
            binaryCursorOffsets[i + 1] = binaryCursorOffsets[i] + this->m_codecParams.attrParams[i].binaryCount;
            if (binaryCursorOffsets[i + 1] < binaryCursorOffsets[i] ||
                binaryCursorOffsets[i + 1] > static_cast<IGsize>(uCharBuffer.size())) {
                IGAME_CORE_ERROR("Invalid IGC attribute payload");
                return false;
            }
        }
        if (binaryCursorOffsets.back() != static_cast<IGsize>(uCharBuffer.size())) {
            IGAME_CORE_ERROR("Invalid IGC attribute payload");
            return false;
        }

        std::vector<AttributeBuffer> attrBuffers(this->m_codecParams.attrParams.size());
        std::atomic_bool attrDecodeFailed{false};
        auto containsInf = [](const auto& values) {
            return std::any_of(values.begin(), values.end(), [](auto value) {
                return std::isinf(value);
            });
        };

        // 进度控制
        CodecProgressParallelFor(this, 0,
                            static_cast<int>(this->m_codecParams.attrParams.size()),
                            m_DecompressProgress, m_DecompressProgress + 0.2f,
                            [&](int start, int end) -> void {
            for (int i = start; i < end; i++) {
                if (attrDecodeFailed.load()) { return; }
                auto& params = this->m_codecParams.attrParams[i];

                // 参数有效性检查
                if (params.dimension <= 0 || params.elementCount <= 0) {
                    continue;
                }

            std::vector<unsigned char> inputBuffer(
                uCharBuffer.begin() + binaryCursorOffsets[i],
                uCharBuffer.begin() + binaryCursorOffsets[i + 1]
            );

            const IGsize valueCount = params.elementCount * params.dimension;

            AttributeBuffer attr;
            attr.name = params.name;
            attr.type = params.type;
            attr.attachmentType = params.attachmentType;
            attr.dimension = params.dimension;
            attr.valueSize = static_cast<int>(params.valueSize);
            attr.arrayType = params.arrayType;
            attr.attrCodec = params.attrCodec;

            bool decodeOk = false;
            if (params.attrCodec == AttrCodec::Unsupported) {
                IGAME_CORE_WARN("Skipping IGC attribute {}: unsupported attribute codec", params.name);
                continue;
            }

            if (params.attrCodec == AttrCodec::FloatMeshopt) {
                const bool legacyInterleaved =
                        m_ParamVersion == LegacyParamVersion &&
                        params.attrLayout == AttrLayout::InterleavedRecord;
                const bool componentSeries = params.attrLayout == AttrLayout::ComponentSeries;
                if (!legacyInterleaved && !componentSeries) {
                    IGAME_CORE_WARN("Skipping IGC attribute {}: unsupported attribute layout", params.name);
                    continue;
                }
                if (params.arrayType == IG_FloatArray) {
                    std::vector<float> floats;
                    MeshFloatCodec::FloatDecoder(floats, inputBuffer, params);
                    // 解码结果大小验证
                    if (floats.size() != valueCount) {
                        attrDecodeFailed.store(true);
                        return;
                    }
                    if (containsInf(floats)) {
                        IGAME_CORE_WARN("IGC attribute '{}' at index {} contains Inf; continuing decode",
                                        params.name, i);
                    }
                    attr.floatData = std::move(floats);
                } else if (params.arrayType == IG_DoubleArray) {
                    std::vector<double> doubles;
                    MeshFloatCodec::FloatDecoder(doubles, inputBuffer, params);
                    // 解码结果大小验证
                    if (doubles.size() != valueCount) {
                        attrDecodeFailed.store(true);
                        return;
                    }
                    if (containsInf(doubles)) {
                        IGAME_CORE_WARN("IGC attribute '{}' at index {} contains Inf; continuing decode",
                                        params.name, i);
                    }
                    attr.doubleData = std::move(doubles);
                } else {
                    IGAME_CORE_WARN("Skipping IGC attribute {}: unsupported float array type {}", params.name, params.arrayType);
                    continue;
                }
            } else if (params.attrCodec == AttrCodec::IntegerDeltaRleVarint) {
                if (params.attrLayout != AttrLayout::ComponentSeries) {
                    IGAME_CORE_WARN("Skipping IGC attribute {}: unsupported integer attribute layout", params.name);
                    continue;
                }
                if (!IsSupportedIntegerAttributeArray(params.arrayType)) {
                    IGAME_CORE_WARN("Skipping IGC attribute {}: unsupported integer array type {}", params.name, params.arrayType);
                    continue;
                }
                decodeOk = DecodeIntegerAttributeByType(attr, inputBuffer, params);
                if (!decodeOk || attr.rawData.size() != static_cast<size_t>(valueCount) * static_cast<size_t>(params.valueSize)) {
                    IGAME_CORE_ERROR("Invalid IGC attribute payload");
                    return false;
                }
            } else {
                IGAME_CORE_WARN("Skipping IGC attribute {}: unsupported attribute codec", params.name);
                continue;
            }

            attr.valid = true;
            attrBuffers[i] = std::move(attr);
            if (!this->m_codecParams.attrParams.empty()) {
                UpdateProgress(m_DecompressProgress + 0.2f * (static_cast<float>(i + 1) /
                                                              static_cast<float>(this->m_codecParams.attrParams.size())));
            }
        }

        // 更新最终进度
        m_DecompressProgress += 0.2;
        UpdateProgress(m_DecompressProgress);

        // 所有线程完成后，按顺序添加属性
        for (const auto& attr : attrBuffers) {
            this->m_DecoderAdapter->AddAttribute(attr);
        }
        return true;
    }

    bool TopoDecoder(PayloadBuffer& buf) {
        if (this->m_codecParams.meshType == IG_POINT_SET) {
            // 点云没有拓扑结构，直接更新进度并返回
            m_DecompressProgress += 0.2;
            UpdateProgress(m_DecompressProgress);
            return true;
        }

        // 结构化网格：不需要解码cell连接关系，通过axisSize自动生成
        if (this->m_codecParams.meshType == IG_STRUCTURED_MESH) {
            this->m_DecoderAdapter->SetStructuredMeshDimension(this->m_codecParams.structuredMeshParams.axisSize);

            m_DecompressProgress += 0.2;
            UpdateProgress(m_DecompressProgress);
            return true;
        }

        std::vector<unsigned char> inputTopo(buf.size());
        std::memcpy(inputTopo.data(), buf.data(), buf.size());

        int inputCursor = 0;
        const auto& topoParams = this->m_codecParams.topoParams;
        const IGsize topoPayloadSize = static_cast<IGsize>(inputTopo.size());
        const IGsize maxInt = static_cast<IGsize>(std::numeric_limits<int>::max());
        if (topoParams.topCellBufferBinaryCount > maxInt ||
            topoParams.topCellSizeBinaryCount > maxInt ||
            topoParams.bottomCellBufferBinaryCount > maxInt ||
            topoParams.bottomCellSizeBinaryCount > maxInt ||
            topoParams.cellTypeBinaryCount > maxInt) {
            IGAME_CORE_ERROR("Invalid IGC topology payload");
            return false;
        }
        IGsize expectedTopoBytes = 0;
        const IGsize topoParts[] = {
            topoParams.topCellBufferBinaryCount,
            topoParams.topCellSizeBinaryCount,
            topoParams.bottomCellBufferBinaryCount,
            topoParams.bottomCellSizeBinaryCount,
            topoParams.cellTypeBinaryCount,
        };
        for (IGsize part : topoParts) {
            if (AddWillOverflow(expectedTopoBytes, part)) {
                IGAME_CORE_ERROR("Invalid IGC topology payload");
                return false;
            }
            expectedTopoBytes += part;
        }
        if (expectedTopoBytes > topoPayloadSize) {
            IGAME_CORE_ERROR("Invalid IGC topology payload");
            return false;
        }

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
        if (inputCursor != static_cast<int>(inputTopo.size())) {
            IGAME_CORE_ERROR("Invalid IGC topology payload");
            return false;
        }
        return true;
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
