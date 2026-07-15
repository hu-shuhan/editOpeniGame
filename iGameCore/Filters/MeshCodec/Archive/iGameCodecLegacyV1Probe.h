#ifndef IGAMEVIS_IGAMECODECLEGACYV1PROBE_H
#define IGAMEVIS_IGAMECODECLEGACYV1PROBE_H

#include "MeshCodec/Utils/iGameMeshCodecParams.h"
#include "iGameMacro.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

class CodecLegacyV1Probe {
public:
    struct Layout {
        const char* name = "";
        size_t boolBytes = 1;
        size_t enumBytes = 4;
        size_t longBytes = 4;
        size_t igSizeBytes = 8;
        size_t igSizeAlignment = 8;
        size_t vectorSizeBytes = 8;
        size_t stringSizeBytes = 8;
    };

    static bool Decode(const std::vector<uint8_t>& data, CodecStorageParams& params, std::string* error = nullptr) {
        const Layout layouts[] = {
            NativeLayout(),
            Win32Layout(),
            Posix64Layout(),
        };

        for (const auto& layout : layouts) {
            Reader reader(data, layout);
            CodecStorageParams candidate;
            std::string validateError;
            if (reader.Read(candidate) && reader.ConsumedAll() && Validate(candidate, &validateError)) {
                params = std::move(candidate);
                return true;
            }
        }

        if (error) { *error = "Legacy v1 parameter payload is incompatible with this platform"; }
        return false;
    }

private:
    static Layout NativeLayout() {
        Layout layout{};
        layout.name = "native";
        layout.boolBytes = sizeof(bool);
        layout.enumBytes = 4;
        layout.longBytes = sizeof(long);
        layout.igSizeBytes = sizeof(IGsize);
        layout.igSizeAlignment = alignof(IGsize);
        layout.vectorSizeBytes = sizeof(uint64_t);
        layout.stringSizeBytes = sizeof(uint64_t);
        return layout;
    }

    static Layout Win32Layout() {
        Layout layout{};
        layout.name = "win32";
        layout.boolBytes = 1;
        layout.enumBytes = 4;
        layout.longBytes = 4;
        layout.igSizeBytes = 8;
        layout.igSizeAlignment = 4;
        layout.vectorSizeBytes = 8;
        layout.stringSizeBytes = 8;
        return layout;
    }

    static Layout Posix64Layout() {
        Layout layout{};
        layout.name = "posix64";
        layout.boolBytes = 1;
        layout.enumBytes = 4;
        layout.longBytes = 8;
        layout.igSizeBytes = 8;
        layout.igSizeAlignment = 8;
        layout.vectorSizeBytes = 8;
        layout.stringSizeBytes = 8;
        return layout;
    }

    class Reader {
    public:
        Reader(const std::vector<uint8_t>& data, Layout layout) : m_Data(data), m_Layout(layout) {}

        bool Read(CodecStorageParams& params) {
            return Read(params.meshType) &&
                   Read(params.structuredMeshParams) &&
                   Read(params.geomParams) &&
                   Read(params.topoParams) &&
                   Read(params.attrParams);
        }

        bool ConsumedAll() {
            if (m_Cursor == m_Data.size()) { return true; }
            SetError("Legacy v1 payload has trailing bytes");
            return false;
        }

        const std::string& Error() const { return m_Error; }

    private:
        bool Read(FloatStorageParams& params) {
            return Read(params.lossyMode) &&
                   Read(params.errorMode) &&
                   Read(params.valueSize) &&
                   Read(params.elementCount) &&
                   Read(params.dimension);
        }

        bool Read(GeomStorageParams& params) {
            return ReadRawFloatStorage(static_cast<FloatStorageParams&>(params));
        }

        bool Read(AttrStorageParams& params) {
            return Read(static_cast<FloatStorageParams&>(params)) &&
                   Read(params.name) &&
                   Read(params.type) &&
                   Read(params.attachmentType) &&
                   Read(params.binaryCount);
        }

        bool Read(TopoStorageParameters& params) {
            const size_t objectStart = m_Cursor;
            return Read(params.isSecondaryIndex) &&
                   AlignToObject(objectStart, alignof(int)) &&
                   Read(params.fixedCellSize) &&
                   AlignToObject(objectStart, m_Layout.igSizeAlignment) &&
                   Read(params.topCellBufferBinaryCount) &&
                   Read(params.topCellSizeBinaryCount) &&
                   Read(params.topCellBufferSize) &&
                   AlignToObject(objectStart, alignof(int)) &&
                   Read(params.topCellBufferPadding) &&
                   AlignToObject(objectStart, m_Layout.igSizeAlignment) &&
                   Read(params.bottomCellBufferBinaryCount) &&
                   Read(params.bottomCellSizeBinaryCount) &&
                   Read(params.bottomCellBufferSize) &&
                   AlignToObject(objectStart, alignof(int)) &&
                   Read(params.bottomCellBufferPadding) &&
                   AlignToObject(objectStart, m_Layout.igSizeAlignment) &&
                   Read(params.cellTypeBinaryCount) &&
                   AlignToObject(objectStart, RawStructAlignment());
        }

        bool Read(StructuredMeshStorageParameters& params) {
            for (int& axis : params.axisSize) {
                if (!Read(axis)) { return false; }
            }
            return true;
        }

        bool Read(std::vector<AttrStorageParams>& target) {
            uint64_t size = 0;
            if (!ReadUnsigned(m_Layout.vectorSizeBytes, size)) { return false; }
            if (size > static_cast<uint64_t>(target.max_size())) {
                SetError("Legacy v1 attr vector is too large for this platform");
                return false;
            }
            target.resize(static_cast<size_t>(size));
            for (auto& item : target) {
                if (!Read(item)) { return false; }
            }
            return true;
        }

        bool Read(std::string& target) {
            uint64_t size = 0;
            if (!ReadUnsigned(m_Layout.stringSizeBytes, size)) { return false; }
            if (size > static_cast<uint64_t>(target.max_size())) {
                SetError("Legacy v1 string is too large for this platform");
                return false;
            }
            if (!Ensure(static_cast<size_t>(size))) { return false; }
            target.assign(reinterpret_cast<const char*>(m_Data.data() + m_Cursor), static_cast<size_t>(size));
            m_Cursor += static_cast<size_t>(size);
            return true;
        }

        bool Read(bool& value) {
            uint64_t raw = 0;
            if (!ReadUnsigned(m_Layout.boolBytes, raw)) { return false; }
            value = raw != 0;
            return true;
        }

        bool Read(LossyMode& value) {
            int64_t raw = 0;
            if (!ReadSigned(m_Layout.enumBytes, raw)) { return false; }
            value = static_cast<LossyMode>(raw);
            return true;
        }

        bool Read(QuantizeMode& value) {
            int64_t raw = 0;
            if (!ReadSigned(m_Layout.enumBytes, raw)) { return false; }
            value = static_cast<QuantizeMode>(raw);
            return true;
        }

        bool Read(IGenum& value) {
            int64_t raw = 0;
            if (!ReadSigned(m_Layout.longBytes, raw)) { return false; }
            if (raw < static_cast<int64_t>(std::numeric_limits<IGenum>::min()) ||
                raw > static_cast<int64_t>(std::numeric_limits<IGenum>::max())) {
                SetError("Legacy v1 IGenum is out of range");
                return false;
            }
            value = static_cast<IGenum>(raw);
            return true;
        }

        bool Read(IGsize& value) {
            uint64_t raw = 0;
            if (!ReadUnsigned(m_Layout.igSizeBytes, raw)) { return false; }
            if (raw > static_cast<uint64_t>(std::numeric_limits<IGsize>::max())) {
                SetError("Legacy v1 IGsize is out of range");
                return false;
            }
            value = static_cast<IGsize>(raw);
            return true;
        }

        bool Read(int& value) {
            int64_t raw = 0;
            if (!ReadSigned(4, raw)) { return false; }
            if (raw < static_cast<int64_t>(std::numeric_limits<int>::min()) ||
                raw > static_cast<int64_t>(std::numeric_limits<int>::max())) {
                SetError("Legacy v1 int is out of range");
                return false;
            }
            value = static_cast<int>(raw);
            return true;
        }

        bool ReadRawFloatStorage(FloatStorageParams& params) {
            const size_t objectStart = m_Cursor;
            return Read(params.lossyMode) &&
                   Read(params.errorMode) &&
                   AlignToObject(objectStart, m_Layout.igSizeAlignment) &&
                   Read(params.valueSize) &&
                   Read(params.elementCount) &&
                   AlignToObject(objectStart, alignof(int)) &&
                   Read(params.dimension) &&
                   AlignToObject(objectStart, RawStructAlignment());
        }

        bool ReadUnsigned(size_t byteCount, uint64_t& value) {
            if (!Ensure(byteCount)) { return false; }
            value = 0;
            std::memcpy(&value, m_Data.data() + m_Cursor, byteCount);
            m_Cursor += byteCount;
            return true;
        }

        bool ReadSigned(size_t byteCount, int64_t& value) {
            uint64_t raw = 0;
            if (!ReadUnsigned(byteCount, raw)) { return false; }
            if (byteCount == 8) {
                value = static_cast<int64_t>(raw);
                return true;
            }
            const uint64_t signBit = uint64_t{1} << (byteCount * 8 - 1);
            const uint64_t mask = (~uint64_t{0}) << (byteCount * 8);
            value = static_cast<int64_t>((raw & signBit) ? (raw | mask) : raw);
            return true;
        }

        bool Ensure(size_t byteCount) {
            if (byteCount > m_Data.size() || m_Cursor > m_Data.size() - byteCount) {
                SetError("Legacy v1 payload ended unexpectedly");
                return false;
            }
            return true;
        }

        bool AlignToObject(size_t objectStart, size_t alignment) {
            if (alignment <= 1) { return true; }
            const size_t objectOffset = m_Cursor - objectStart;
            const size_t padding = (alignment - objectOffset % alignment) % alignment;
            if (!Ensure(padding)) { return false; }
            m_Cursor += padding;
            return true;
        }

        size_t RawStructAlignment() const {
            return std::max<size_t>(alignof(int), m_Layout.igSizeAlignment);
        }

        void SetError(const char* error) {
            if (m_Error.empty()) {
                m_Error = m_Layout.name;
                m_Error += ": ";
                m_Error += error;
            }
        }

        const std::vector<uint8_t>& m_Data;
        Layout m_Layout;
        size_t m_Cursor = 0;
        std::string m_Error;
    };

    static bool AddWillOverflow(IGsize a, IGsize b) {
        return b > std::numeric_limits<IGsize>::max() - a;
    }

    static bool Validate(const CodecStorageParams& params, std::string* error) {
        auto fail = [&](const char* message) {
            if (error) { *error = message; }
            return false;
        };

        switch (params.meshType) {
            case IG_POINT_SET:
            case IG_SURFACE_MESH:
            case IG_VOLUME_MESH:
            case IG_STRUCTURED_MESH:
            case IG_UNSTRUCTURED_MESH:
                break;
            default:
                return fail("Legacy v1 mesh type is invalid");
        }

        if (params.geomParams.valueSize != sizeof(float)) { return fail("Legacy v1 geometry value size is invalid"); }
        if (params.geomParams.dimension != 3) { return fail("Legacy v1 geometry dimension is invalid"); }
        if constexpr (sizeof(std::size_t) < sizeof(uint64_t)) {
            if (CodecStorageParamSizeLimits::ParamsExceed32Bit(params)) {
                return fail("Legacy v1 parameter payload is incompatible with this platform");
            }
        }

        IGsize attrBinaryTotal = 0;
        for (const auto& attr : params.attrParams) {
            if (attr.dimension <= 0) { return fail("Legacy v1 attribute dimension is invalid"); }
            if (attr.valueSize != sizeof(float) && attr.valueSize != sizeof(double)) {
                return fail("Legacy v1 attribute value size is invalid");
            }
            if (attr.type < IG_SCALAR || attr.type >= IG_ATTRIBUTE_COUNT) {
                return fail("Legacy v1 attribute type is invalid");
            }
            if (attr.attachmentType < IG_POINT || attr.attachmentType > IG_MID_POINT) {
                return fail("Legacy v1 attribute attachment type is invalid");
            }
            if (AddWillOverflow(attrBinaryTotal, attr.binaryCount)) {
                return fail("Legacy v1 attribute binary count overflows");
            }
            attrBinaryTotal += attr.binaryCount;
        }

        const auto& topo = params.topoParams;
        IGsize topoTotal = 0;
        const IGsize topoParts[] = {
            topo.topCellBufferBinaryCount,
            topo.topCellSizeBinaryCount,
            topo.bottomCellBufferBinaryCount,
            topo.bottomCellSizeBinaryCount,
            topo.cellTypeBinaryCount,
        };
        for (IGsize part : topoParts) {
            if (AddWillOverflow(topoTotal, part)) { return fail("Legacy v1 topology binary count overflows"); }
            topoTotal += part;
        }

        return true;
    }
};

IGAME_NAMESPACE_END

#endif
