//
// Created by m_ky on 2024/11/28.
//

/**
 * @class   iGameImageWriter
 * @brief   iGameImageWriter's brief
 */

#include "iGameImageWriter.h"
IGAME_NAMESPACE_BEGIN
void ImageWriter::SetInputBuffer(std::vector<uint8_t>& buffer_data) {
    m_BufferLength = buffer_data.size();
    m_BufferData = buffer_data.data();
}

void ImageWriter::SetInputBuffer(UnsignedCharArray::Pointer buffer_data) {
    m_BufferLength = buffer_data->GetCapacity();
    m_BufferData = buffer_data->RawPointer();
}

void ImageWriter::SetOutputFilePath(const std::string &filePath) {
    m_OutputFilePath = filePath.c_str();
}

void ImageWriter::SetOutputFilePath(const char *filePath) {
    m_OutputFilePath = filePath;
}

bool ImageWriter::Execute() {
    std::cout << "No specific writer is specified\n";
    return Filter::Execute();
}

void ImageWriter::SetOutPutImageSize(int &width, int &height) {
    m_IMG_width = width;
    m_IMG_height = height;
}

IGAME_NAMESPACE_END
