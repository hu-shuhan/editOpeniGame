#ifndef iGameMeshDataSourceMemoryMapped_h
#define iGameMeshDataSourceMemoryMapped_h

#include "MeshCodec/DecodeInput/iGameIDecodeInput.h"

IGAME_NAMESPACE_BEGIN

class DecodeInputMappedMemory final : public IDecodeInput {
public:
    I_OBJECT(DecodeInputMappedMemory);

    static Pointer New() { return new DecodeInputMappedMemory; }

    static Pointer New(char* fileStart, const char* currentPos, char* fileEnd, size_t fileSize) {
        auto p = new DecodeInputMappedMemory;
        p->SetData(fileStart, currentPos, fileEnd, fileSize);
        return p;
    }

    void SetData(char* fileStart, const char* currentPos, char* fileEnd, size_t fileSize) {
        m_FileStart = fileStart;
        m_CurrentPos = currentPos;
        m_FileEnd = fileEnd;
        m_FileSize = fileSize;
    }

    bool Initialize() override {
        if (!m_FileStart) { return false; }
        if (!m_FileEnd) { return false; }
        if (m_CurrentPos >= m_FileEnd) { return false; }
        if (m_FileSize == 0) { return false; }
        return true;
    }

    bool ReadPayload(PayloadBuffer* buf) override {
        buf->resize(0);

        if (m_CurrentPos + 5 > m_FileEnd) { return false; }

        buf->type = static_cast<PayloadType>(*m_CurrentPos++);

        uint32_t length = 0;
        length = (length << 8) | static_cast<unsigned char>(*m_CurrentPos++);
        length = (length << 8) | static_cast<unsigned char>(*m_CurrentPos++);
        length = (length << 8) | static_cast<unsigned char>(*m_CurrentPos++);
        length = (length << 8) | static_cast<unsigned char>(*m_CurrentPos++);

        if (m_CurrentPos + length > m_FileEnd) { return false; }

        buf->resize(length);
        std::memcpy(buf->data(), m_CurrentPos, length);
        m_CurrentPos += length;

        return true;
    }

protected:
    DecodeInputMappedMemory() = default;

private:
    const char* m_FileStart = nullptr;
    const char* m_CurrentPos = nullptr;
    const char* m_FileEnd = nullptr;
    size_t m_FileSize = 0;
};

IGAME_NAMESPACE_END
#endif
