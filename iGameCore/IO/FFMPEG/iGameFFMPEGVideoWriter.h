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
    int bytes_per_line;
    std::string output_path;
    std::vector<std::vector<uint8_t>> raw_image_data;
};

class FFMPEGVideoWriterInternal;
class FFMPEGVideoWriter : public Filter{
public:
    I_OBJECT(FFMPEGVideoWriter)

    static FFMPEGVideoWriter::Pointer New();

    void SetImageData(const  std::vector<std::pair<int, std::vector<uint8_t>>>& _imageData_vec);

    void SetOutPutPath(const std::string& path);

    void SetImageSize(int width, int height);

    void SetVideoInputInfo(VideoInputInfo& info);

    bool Execute() override;



protected:
    std::vector<std::pair<int, std::vector<uint8_t>>> m_RawImageData;
    std::string m_videoStoragePath;
    int m_Width{1920}, m_Height{1080};

    FFMPEGVideoWriterInternal* m_Internal;
    VideoInputInfo m_VideoInfo;
protected:
    FFMPEGVideoWriter();
    ~FFMPEGVideoWriter() override;



};
IGAME_NAMESPACE_END

#endif
