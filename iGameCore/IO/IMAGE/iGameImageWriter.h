/**
 * @class   iGameImageWriter
 * @brief   iGameImageWriter's brief
 */

#pragma once

#include <iGameFilter.h>
#include <iGameFlatArray.h>
IGAME_NAMESPACE_BEGIN
class ImageWriter : public Filter{
public:
    I_OBJECT(ImageWriter)

    void SetOutputFilePath(const std::string& filePath);
    void SetOutputFilePath(const char* filePath);
    virtual void SetInputBuffer(std::vector<uint8_t>& buffer_data);
    virtual void SetInputBuffer(UnsignedCharArray::Pointer buffer_data);

    virtual void SetOutPutImageSize(int& width, int& height);
    bool Execute() override;

protected:
    ImageWriter() = default;
    ~ImageWriter() = default;

    int m_IMG_width{-1}, m_IMG_height{-1};
    uint8_t* m_BufferData {nullptr};
    uint64_t m_BufferLength {0};
    const char* m_OutputFilePath{nullptr};
};
IGAME_NAMESPACE_END
