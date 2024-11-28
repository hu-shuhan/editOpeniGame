/**
 * @class   iGameBMPWriter
 * @brief   iGameBMPWriter's brief
 */

#pragma once

#include "iGameImageWriter.h"

IGAME_NAMESPACE_BEGIN
class BMPWriter : public ImageWriter{
public:
    I_OBJECT(BMPWriter)
    static Pointer New(){return new BMPWriter;}

    bool Execute() override;

protected:
    BMPWriter() = default;
    ~BMPWriter() = default;
};
IGAME_NAMESPACE_END