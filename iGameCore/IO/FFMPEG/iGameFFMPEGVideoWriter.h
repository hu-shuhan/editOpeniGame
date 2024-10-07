/**
 * @class   iGameFFMPEGVideoWriter
 * @brief   iGameFFMPEGVideoWriter's brief
 */

#ifdef FFMPEG_ENABLE
#pragma once
#include <iGameObject.h>

IGAME_NAMESPACE_BEGIN
class FFMPEGVideoWriter : public Object{
public:
    I_OBJECT(FFMPEGVideoWriter)

    static FFMPEGVideoWriter::Pointer New();

    void SetImageData(const std::vector<const char*>& _imageData_vec);


protected:
    std::vector<const char*> m_RawImageData;

protected:
    FFMPEGVideoWriter();
    ~FFMPEGVideoWriter() override = default;



};
IGAME_NAMESPACE_END

#endif
