/**
 * @class   iGameFFMPEGVideoWriter
 * @brief   iGameFFMPEGVideoWriter's brief
 */

#if defined(FFMPEG_ENABLE)
#pragma once
#include <iGameFilter.h>

IGAME_NAMESPACE_BEGIN
struct VideoInputInfo{
    int width;
    int height;
    int frame_rate;
    int bit_rate;

    std::string output_path;
    int bytes_per_line;
    std::vector<std::vector<uint8_t>> raw_image_data;
};

class FFMPEGVideoWriterInternal;
class FFMPEGVideoWriter : public Filter{
public:
    I_OBJECT(FFMPEGVideoWriter)

    static FFMPEGVideoWriter::Pointer New();

    void SetVideoInputInfo(VideoInputInfo& info);

    bool Execute() override;

protected:

    FFMPEGVideoWriterInternal* m_Internal;
    VideoInputInfo m_VideoInfo;
protected:
    FFMPEGVideoWriter();
    ~FFMPEGVideoWriter() override;



};
IGAME_NAMESPACE_END

#endif
