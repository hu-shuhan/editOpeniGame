#ifndef iGameEncoderOutputBinaryArray_h
#define iGameEncoderOutputBinaryArray_h

#include "MeshCodec/EncodeOutput/iGameIEncodeOutput.h"
#include <cstring>
#include <vector>

IGAME_NAMESPACE_BEGIN

class EncodeOutputBinaryArray final : public IEncodeOutput {
public:
    I_OBJECT(EncodeOutputBinaryArray);

    static Pointer New() { return new EncodeOutputBinaryArray; }

    void WritePayload(const PayloadBuffer& buf) override {
        auto length = static_cast<uint32_t>(buf.size());

        // 计算总需要的字节数: 1字节类型 + 4字节长度 + payload数据
        size_t totalSize = 1 + 4 + length;

        // 获取当前大小，作为新数据的起始位置
        size_t currentSize = m_Buffers.size();

        // 扩展以容纳新数据
        m_Buffers.resize(currentSize + totalSize);

        // 获取数据指针
        unsigned char* data = m_Buffers.data() + currentSize;

        // 写入 header (类型 + 长度)
        WritePayloadHeader(data, buf.type, length);

        // 写入 payload 数据
        if (length > 0) {
            std::memcpy(data + 5, buf.data(), length);
        }
    }

    size_t GetSize() const override { return m_Buffers.size(); }

    const unsigned char* GetData() const { return m_Buffers.data(); }
    std::vector<unsigned char>& GetBuffers() { return m_Buffers; }
    const std::vector<unsigned char>& GetBuffers() const { return m_Buffers; }

protected:
    EncodeOutputBinaryArray() = default;

private:
    std::vector<unsigned char> m_Buffers;

    void WritePayloadHeader(unsigned char* dest, PayloadType type, uint32_t length) {
        dest[0] = static_cast<unsigned char>(type);
        dest[1] = static_cast<unsigned char>(length >> 24);
        dest[2] = static_cast<unsigned char>(length >> 16);
        dest[3] = static_cast<unsigned char>(length >> 8);
        dest[4] = static_cast<unsigned char>(length >> 0);
    }
};

IGAME_NAMESPACE_END
#endif
