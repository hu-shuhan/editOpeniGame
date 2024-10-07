//
// Created by m_ky on 2024/10/5.
//
#ifdef FFMPEG_ENABLE

/**
 * @class   iGameFFMPEGVideoWriter
 * @brief   iGameFFMPEGVideoWriter's brief
 */

#include "iGameFFMPEGVideoWriter.h"
//extern "C" {
//#include <libavcodec/avcodec.h>
//}


IGAME_NAMESPACE_BEGIN
FFMPEGVideoWriter::FFMPEGVideoWriter() {
//    AVFormatContext *kAVFormatContext;
//    AVOutputFormat *kAVOutputFormat;
//    AVCodecContext *kAVCodecContext;
//    AVDictionary *kAVDictionary = NULL;
//    AVDictionary *opt = NULL;
//    AVCodec *video_codec;
////    AVStream *kAVStream;
//    AVFrame *frame;
//    AVFrame *tmp_frame;
//    AVPacket kAVPacket = { 0 };
//    struct SwsContext *kSwsContext;
//    AVRational time{1,24};  /*1s25帧*/
//    AVRational time_1{1,1};  /*1s25帧*/
//    uint8_t *intBuffer = 0;
//
//    int ret;
//    int got_packer = 0;
//    int64_t next_pts = 0;
//
//    tmp_frame = av_frame_alloc();
//    av_init_packet(&kAVPacket);
//
//    av_register_all();

}

FFMPEGVideoWriter::Pointer FFMPEGVideoWriter::New() {
    return new FFMPEGVideoWriter();
}

void FFMPEGVideoWriter::SetImageData(const std::vector<const char *> &_imageData_vec) {
    m_RawImageData = _imageData_vec;
}
IGAME_NAMESPACE_END
#endif