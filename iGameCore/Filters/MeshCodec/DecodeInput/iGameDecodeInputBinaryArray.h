#ifndef iGameMeshDataSourceBinary_h
#define iGameMeshDataSourceBinary_h

#include "MeshCodec/DecodeInput/iGameIDecodeInput.h"

IGAME_NAMESPACE_BEGIN

class DecodeInputBinaryArray final : public IDecodeInput {
public:
    I_OBJECT(DecodeInputBinaryArray);

    static Pointer New() { return new DecodeInputBinaryArray; }

    static Pointer New(const unsigned char* data, size_t size) {
        auto p = new DecodeInputBinaryArray;
        p->SetData(data, size);
        return p;
    }

    void SetData(const unsigned char* data, size_t size) {
        m_Data = data;
        m_Size = size;
        m_Offset = 0;
    }

    bool Initialize() override {
        if (m_Data == nullptr || m_Size == 0) { return false; }
        m_Offset = 0;
        return true;
    }

    bool ReadPayload(PayloadBuffer* buf) override {
        buf->resize(0);

        uint32_t length = 0;
        if (!ReadPayloadHeader(m_Data, m_Size, m_Offset, buf, length)) { return false; }
        if (!ReadPayloadData(m_Data, m_Size, m_Offset, buf, length)) { return false; }

        return true;
    }

protected:
    DecodeInputBinaryArray() = default;

private:
    const unsigned char* m_Data = nullptr;
    size_t m_Size = 0;
    size_t m_Offset = 0;

    bool ReadPayloadHeader(const unsigned char* data, size_t dataSize, size_t& offset, PayloadBuffer* buf,
                       uint32_t& outLength) {
        if (offset + 5 > dataSize) { return false; }

        buf->type = static_cast<PayloadType>(data[offset++]);

        outLength = 0;
        outLength = (outLength << 8) | data[offset++];
        outLength = (outLength << 8) | data[offset++];
        outLength = (outLength << 8) | data[offset++];
        outLength = (outLength << 8) | data[offset++];

        return true;
    }

    bool ReadPayloadData(const unsigned char* data, size_t dataSize, size_t& offset, PayloadBuffer* buf,
                         uint32_t length) {
        if (offset + length > dataSize) { return false; }

        buf->resize(length);
        if (length > 0) {
            std::memcpy(buf->data(), data + offset, length);
            offset += length;
        }
        return true;
    }
};

IGAME_NAMESPACE_END
#endif
