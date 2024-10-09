//
// Created by m_ky on 2024/10/5.
//
/**
 * @class   iGameFFMPEGVideoWriter
 * @brief   iGameFFMPEGVideoWriter's brief
 */

#if defined(FFMPEG_ENABLE)

#include "iGameFFMPEGVideoWriter.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

IGAME_NAMESPACE_BEGIN
FFMPEGVideoWriter::Pointer FFMPEGVideoWriter::New() {
    return new FFMPEGVideoWriter;
}

void FFMPEGVideoWriter::SetVideoInputInfo(VideoInputInfo &info) {
    m_VideoInfo = std::move(info);
}

class FFMPEGVideoWriterInternal{
public:
    FFMPEGVideoWriterInternal(){
//        av_codec_r
    };


public:
//    AVFormatContext* formatContext = nullptr;
//    AVCodecContext* codecContext = nullptr;
//    AVStream* videoStream = nullptr;
//    AVFrame* frame = nullptr;
//    SwsContext* swsContext = nullptr;
//    int width{1920};
//    int height{1080};
//    int fps{1};
//    int frameIndex = 0;

};
FFMPEGVideoWriter::FFMPEGVideoWriter() {
    m_Internal = new FFMPEGVideoWriterInternal;
}

FFMPEGVideoWriter::~FFMPEGVideoWriter() {
    delete m_Internal;
}

bool FFMPEGVideoWriter::SaveMP4() {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVStream* videoStream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket pkt;
    av_init_packet(&pkt);  // 初始化 AVPacket
    pkt.data = nullptr;
    pkt.size = 0;
    SwsContext* swsContext = nullptr;
    int ret;

    const char* storagePath = m_VideoInfo.output_path.c_str();
    // 创建输出格式
    avformat_alloc_output_context2(&formatContext, nullptr, "mp4", storagePath);
    if (!formatContext) {
        std::cout << "Could not create output context";
        return false;
    }

    // 选择编码器
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cout  << "Codec not found";
        return false;
    }

    // 创建视频流
    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream) {
        std::cout  << "Could not create video stream";
        return false;
    }
    // 设置编码器参数
    int width = m_VideoInfo.width, height = m_VideoInfo.height;
    codecContext = avcodec_alloc_context3(codec);
    codecContext->width = m_VideoInfo.width;
    codecContext->height = m_VideoInfo.height;
    codecContext->time_base = AVRational{1, m_VideoInfo.frame_rate};
    codecContext->framerate = AVRational{m_VideoInfo.frame_rate, 1};
    codecContext->gop_size = 12; // 每12帧插入一个I帧
    codecContext->max_b_frames = 2;
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext->bit_rate = m_VideoInfo.bit_rate;  // 400kbps
    videoStream->time_base = codecContext->time_base;  // 同步流的时间基
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // 打开编码器
    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0) {
        std::cout  << "Could not open codec";
        return false;
    }

    videoStream->codecpar->codec_id = formatContext->oformat->video_codec;
    videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    videoStream->codecpar->width = codecContext->width;
    videoStream->codecpar->height = codecContext->height;
    videoStream->codecpar->format = codecContext->pix_fmt;
    avcodec_parameters_from_context(videoStream->codecpar, codecContext);

    // 打开输出文件
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext->pb, storagePath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cout  << "Could not open output file";
            return false;
        }
    }

    // 写入文件头
    ret = avformat_write_header(formatContext, nullptr);
    if (ret < 0) {
        std::cout  << "Error occurred when writing header";
        return false;
    }

    // 创建SWS上下文，用于图像格式转换
    swsContext = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

    // 创建帧
    frame = av_frame_alloc();
    frame->format = codecContext->pix_fmt;
    frame->width = codecContext->width;
    frame->height = codecContext->height;
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        std::cout  << "Could not allocate frame data";
        return false;
    }

    for (int i = 0; i < m_VideoInfo.raw_image_data.size(); ++i) {
        // 将uint8_t转换为YUV格式
        uint8_t* inData[1] = { m_VideoInfo.raw_image_data[i].data() };
        int inLinesize[1] = { m_VideoInfo.bytes_per_line };

        sws_scale(swsContext, inData, inLinesize, 0, height, frame->data, frame->linesize);
        frame->pts = i;

        // 发送帧
        ret = avcodec_send_frame(codecContext, frame);
        if (ret < 0) {
            std::cout << "Error sending frame: " << ret << std::endl;
            break;
        }

        // 循环接收编码包
        while (true) {
            ret = avcodec_receive_packet(codecContext, &pkt);
            if (ret == AVERROR(EAGAIN)) {
//                std::cout << "Encoder needs more frames (EAGAIN)." << std::endl;
                break;
            } else if (ret == AVERROR_EOF) {
                std::cout << "Encoder has finished (EOF)." << std::endl;
                break;
            } else if (ret < 0) {
                std::cout << "Error encoding frame: " << ret << std::endl;
                break;
            }
            // 确保包的时间戳正确
            av_packet_rescale_ts(&pkt, codecContext->time_base, videoStream->time_base);
            pkt.stream_index = videoStream->index;
            av_interleaved_write_frame(formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }

// 发送null帧，确保编码器输出所有剩余的包
    ret = avcodec_send_frame(codecContext, nullptr);
    if (ret >= 0) {
        while (true) {
            ret = avcodec_receive_packet(codecContext, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0) {
                std::cout << "Error encoding final frame." << std::endl;
                break;
            }
            // 确保包的时间戳正确
            av_packet_rescale_ts(&pkt, codecContext->time_base, videoStream->time_base);
            pkt.stream_index = videoStream->index;
            av_interleaved_write_frame(formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }
    // 写入文件尾
    av_write_trailer(formatContext);

    // 释放资源
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    sws_freeContext(swsContext);
    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&formatContext->pb);
    avformat_free_context(formatContext);

    return true;
}

bool FFMPEGVideoWriter::SaveGIF() {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVStream* videoStream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket pkt;
    av_init_packet(&pkt);  // 初始化 AVPacket
    pkt.data = nullptr;
    pkt.size = 0;
    SwsContext* swsContext = nullptr;
    int ret;

    const char* storagePath = m_VideoInfo.output_path.c_str();
    // 创建输出格式
    avformat_alloc_output_context2(&formatContext, nullptr, "gif", storagePath);
    if (!formatContext) {
        std::cout << "Could not create output context";
        return false;
    }

    // 选择编码器（GIF 编码器）
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_GIF);
    if (!codec) {
        std::cout << "GIF Codec not found";
        return false;
    }

    // 创建视频流
    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream) {
        std::cout << "Could not create video stream";
        return false;
    }

    // 设置编码器参数
    codecContext = avcodec_alloc_context3(codec);
    codecContext->width = m_VideoInfo.width;
    codecContext->height = m_VideoInfo.height;
    codecContext->time_base = AVRational{1, m_VideoInfo.frame_rate};   // 设置时间基为 1/fps 秒
    codecContext->framerate = AVRational{m_VideoInfo.frame_rate, 1};   // 设置帧率
    codecContext->pix_fmt = AV_PIX_FMT_RGB8; // 使用RGB24格式

    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // 打开编码器
    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0) {
        std::cout << "Could not open codec";
        return false;
    }

    videoStream->codecpar->codec_id = formatContext->oformat->video_codec;
    videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    videoStream->codecpar->width = codecContext->width;
    videoStream->codecpar->height = codecContext->height;
    videoStream->codecpar->format = codecContext->pix_fmt;
    videoStream->time_base = codecContext->time_base;
    avcodec_parameters_from_context(videoStream->codecpar, codecContext);

    // 打开输出文件
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext->pb, storagePath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cout << "Could not open output file";
            return false;
        }
    }

    // 写入文件头
    ret = avformat_write_header(formatContext, nullptr);
    if (ret < 0) {
        std::cout << "Error occurred when writing header";
        return false;
    }


    // 创建SWS上下文，将图像从RGB24转换为GIF所需格式
    swsContext = sws_getContext(m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGB24, m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGB8, SWS_BILINEAR, nullptr, nullptr, nullptr);

    // 创建帧
    frame = av_frame_alloc();
//    frame->format = codecContext->pix_fmt;
    frame->format = AV_PIX_FMT_RGB8;
    frame->width = codecContext->width;
    frame->height = codecContext->height;
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        std::cout << "Could not allocate frame data";
        return false;
    }

    // 循环处理每帧
    for (int i = 0; i < m_VideoInfo.raw_image_data.size(); ++i) {
        // 将uint8_t转换为YUV格式
        uint8_t* inData[1] = { m_VideoInfo.raw_image_data[i].data() };
        int inLinesize[1] = { m_VideoInfo.bytes_per_line };

        // 转换为GIF格式
        sws_scale(swsContext, inData, inLinesize, 0, m_VideoInfo.height, frame->data, frame->linesize);

        // 设置帧的 PTS
        frame->pts = i;

        // 发送帧到编码器
        ret = avcodec_send_frame(codecContext, frame);
        if (ret < 0) {
            std::cout << "Error sending frame: " << ret << std::endl;
            break;
        }

        // 接收编码包
        while (true) {
            ret = avcodec_receive_packet(codecContext, &pkt);
            if (ret == AVERROR(EAGAIN)) {
                break;
            } else if (ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                std::cout << "Error encoding frame: " << ret << std::endl;
                break;
            }

            // 转换时间基
            av_packet_rescale_ts(&pkt, codecContext->time_base, videoStream->time_base);
            pkt.stream_index = videoStream->index;

            // 写入文件
            av_interleaved_write_frame(formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // 发送 null 帧，确保编码器输出所有剩余的包
    ret = avcodec_send_frame(codecContext, nullptr);
    if (ret >= 0) {
        while (true) {
            ret = avcodec_receive_packet(codecContext, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0) {
                std::cout << "Error encoding final frame." << std::endl;
                break;
            }
            pkt.stream_index = videoStream->index;
            av_interleaved_write_frame(formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // 写入文件尾
    av_write_trailer(formatContext);

    // 释放资源
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    sws_freeContext(swsContext);
    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&formatContext->pb);
    avformat_free_context(formatContext);

    return true;
}


IGAME_NAMESPACE_END
#endif