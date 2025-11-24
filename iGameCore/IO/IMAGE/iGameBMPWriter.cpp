//
// Created by m_ky on 2024/11/28.
//

/**
 * @class   iGameBMPWriter
 * @brief   iGameBMPWriter's brief
 */

#include "iGameBMPWriter.h"
#include "Log/iGameLogger.h"

#include <fstream>
#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#endif
bool iGame::BMPWriter::Execute() {
    if(m_IMG_height * m_IMG_width * 3 != m_BufferLength) {
        IGAME_CORE_ERROR("IMG size is not fit with Buffer data.");
        return false;
    }

#if defined(PLATFORM_WINDOWS)
    FILE* pfile = fopen(m_OutputFilePath, "wb");
    if(pfile){
        BITMAPFILEHEADER  bfh;
        memset(&bfh, 0, sizeof (BITMAPFILEHEADER));
        bfh.bfType = 0x4D42;
        bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof (BITMAPINFOHEADER) + m_BufferLength;
        bfh.bfOffBits = sizeof (BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, pfile);

        BITMAPINFOHEADER  bih;
        memset(&bih, 0, sizeof (BITMAPINFOHEADER));
        bih.biWidth = m_IMG_width;
        bih.biHeight = m_IMG_height;
        bih.biBitCount = 24;
        bih.biSize = sizeof(BITMAPINFOHEADER);
        fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, pfile);

        fwrite(m_BufferData, 1, m_IMG_width * m_IMG_height * 3, pfile);
        fclose(pfile);
    }
#else
    IGAME_CORE_WARN("This platform not support BMPWriter currently");
    return false;

#endif
    return true;
}
