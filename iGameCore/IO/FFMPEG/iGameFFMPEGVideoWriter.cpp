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

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
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
//    std::cout << " codec ID " <<  (codec->id == formatContext->oformat->video_codec) << '\n';
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

//    videoStream->codecpar->codec_id = formatContext->oformat->video_codec;
//    videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
//    videoStream->codecpar->width = codecContext->width;
//    videoStream->codecpar->height = codecContext->height;
//    videoStream->codecpar->format = codecContext->pix_fmt;
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
    swsContext = sws_getContext(width, height, AV_PIX_FMT_RGBA,
                                width, height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

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
        frame->pts = av_rescale_q(i, AVRational{1, m_VideoInfo.frame_rate}, codecContext->time_base);

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
    AVFrame* rgbFrame = nullptr;
    AVFrame* palFrame = nullptr;
    AVFrame* paletteFrame = nullptr;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;
    SwsContext* swsContext = nullptr;
    
    // 第一遍：生成调色板的滤镜图
    AVFilterGraph* palettegen_graph = nullptr;
    AVFilterContext* palettegen_src_ctx = nullptr;
    AVFilterContext* palettegen_sink_ctx = nullptr;
    
    // 第二遍：使用调色板的滤镜图
    AVFilterGraph* paletteuse_graph = nullptr;
    AVFilterContext* paletteuse_src_ctx = nullptr;
    AVFilterContext* paletteuse_pal_ctx = nullptr;
    AVFilterContext* paletteuse_sink_ctx = nullptr;
    
    int ret;
    const char* storagePath = m_VideoInfo.output_path.c_str();
    
    std::cout << "Starting GIF encoding..." << std::endl;
    
    // 创建SWS上下文：RGBA -> RGB24
    swsContext = sws_getContext(
        m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGBA,
        m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    if (!swsContext) {
        std::cout << "Could not create swsContext" << std::endl;
        return false;
    }

    // 创建RGB24帧
    rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = m_VideoInfo.width;
    rgbFrame->height = m_VideoInfo.height;
    ret = av_frame_get_buffer(rgbFrame, 32);
    if (ret < 0) {
        std::cout << "Could not allocate RGB frame" << std::endl;
        sws_freeContext(swsContext);
        return false;
    }

    // ========== 第一遍：生成调色板 ==========
    std::cout << "Pass 1: Generating palette..." << std::endl;
    
    palettegen_graph = avfilter_graph_alloc();
    if (!palettegen_graph) {
        std::cout << "Could not create palettegen graph" << std::endl;
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 创建palettegen的buffer source
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/%d:pixel_aspect=1/1",
             m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGB24, m_VideoInfo.frame_rate);
    
    ret = avfilter_graph_create_filter(&palettegen_src_ctx, buffersrc, "in", args, nullptr, palettegen_graph);
    if (ret < 0) {
        std::cout << "Cannot create palettegen buffer source" << std::endl;
        avfilter_graph_free(&palettegen_graph);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 创建palettegen滤镜
    const AVFilter* palettegen = avfilter_get_by_name("palettegen");
    AVFilterContext* palettegen_ctx = nullptr;
    ret = avfilter_graph_create_filter(&palettegen_ctx, palettegen, "palettegen",
                                      "max_colors=256", nullptr, palettegen_graph);
    if (ret < 0) {
        std::cout << "Cannot create palettegen filter" << std::endl;
        avfilter_graph_free(&palettegen_graph);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 创建palettegen的buffer sink
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    ret = avfilter_graph_create_filter(&palettegen_sink_ctx, buffersink, "out", nullptr, nullptr, palettegen_graph);
    if (ret < 0) {
        std::cout << "Cannot create palettegen buffer sink" << std::endl;
        avfilter_graph_free(&palettegen_graph);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 连接滤镜：in -> palettegen -> out
    ret = avfilter_link(palettegen_src_ctx, 0, palettegen_ctx, 0);
    if (ret < 0) {
        std::cout << "Error linking palettegen source" << std::endl;
        avfilter_graph_free(&palettegen_graph);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }
    
    ret = avfilter_link(palettegen_ctx, 0, palettegen_sink_ctx, 0);
    if (ret < 0) {
        std::cout << "Error linking palettegen sink" << std::endl;
        avfilter_graph_free(&palettegen_graph);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 配置palettegen滤镜图
    ret = avfilter_graph_config(palettegen_graph, nullptr);
    if (ret < 0) {
        std::cout << "Error configuring palettegen graph" << std::endl;
        avfilter_graph_free(&palettegen_graph);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 推送所有帧到palettegen
    for (int i = 0; i < m_VideoInfo.raw_image_data.size(); ++i) {
        uint8_t* inData[1] = { m_VideoInfo.raw_image_data[i].data() };
        int inLinesize[1] = { m_VideoInfo.bytes_per_line };
        
        ret = sws_scale(swsContext, inData, inLinesize, 0, m_VideoInfo.height,
                       rgbFrame->data, rgbFrame->linesize);
        if (ret < 0) {
            std::cout << "Error converting RGBA to RGB24" << std::endl;
            break;
        }

        rgbFrame->pts = i;
        ret = av_buffersrc_add_frame_flags(palettegen_src_ctx, rgbFrame, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret < 0) {
            std::cout << "Error feeding frame to palettegen" << std::endl;
            break;
        }
    }

    // 刷新palettegen
    av_buffersrc_add_frame_flags(palettegen_src_ctx, nullptr, 0);

    // 获取生成的调色板
    paletteFrame = av_frame_alloc();
    ret = av_buffersink_get_frame(palettegen_sink_ctx, paletteFrame);
    if (ret < 0) {
        std::cout << "Error getting palette frame" << std::endl;
        av_frame_free(&paletteFrame);
        avfilter_graph_free(&palettegen_graph);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    std::cout << "Palette generated successfully" << std::endl;

    // 释放palettegen滤镜图
    avfilter_graph_free(&palettegen_graph);

    // ========== 第二遍：使用调色板转换帧 ==========
    std::cout << "Pass 2: Converting frames with palette..." << std::endl;
    
    paletteuse_graph = avfilter_graph_alloc();
    if (!paletteuse_graph) {
        std::cout << "Could not create paletteuse graph" << std::endl;
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 创建paletteuse的video buffer source（使用原始视频尺寸）
    char video_args[512];
    snprintf(video_args, sizeof(video_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/%d:pixel_aspect=1/1",
             m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGB24, m_VideoInfo.frame_rate);
    
    ret = avfilter_graph_create_filter(&paletteuse_src_ctx, buffersrc, "main", video_args, nullptr, paletteuse_graph);
    if (ret < 0) {
        std::cout << "Cannot create paletteuse video source" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 创建paletteuse的palette buffer source（使用调色板尺寸）
    char palette_args[512];
    snprintf(palette_args, sizeof(palette_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/%d:pixel_aspect=1/1",
             paletteFrame->width, paletteFrame->height, paletteFrame->format, m_VideoInfo.frame_rate);
    
    ret = avfilter_graph_create_filter(&paletteuse_pal_ctx, buffersrc, "palette", palette_args, nullptr, paletteuse_graph);
    if (ret < 0) {
        std::cout << "Cannot create paletteuse palette source" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 创建paletteuse滤镜
    const AVFilter* paletteuse = avfilter_get_by_name("paletteuse");
    AVFilterContext* paletteuse_ctx = nullptr;
    ret = avfilter_graph_create_filter(&paletteuse_ctx, paletteuse, "paletteuse",
                                      "dither=bayer:bayer_scale=5", nullptr, paletteuse_graph);
    if (ret < 0) {
        std::cout << "Cannot create paletteuse filter" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 创建paletteuse的buffer sink
    ret = avfilter_graph_create_filter(&paletteuse_sink_ctx, buffersink, "out", nullptr, nullptr, paletteuse_graph);
    if (ret < 0) {
        std::cout << "Cannot create paletteuse buffer sink" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 连接滤镜：main -> paletteuse[0], palette -> paletteuse[1], paletteuse -> out
    ret = avfilter_link(paletteuse_src_ctx, 0, paletteuse_ctx, 0);
    if (ret < 0) {
        std::cout << "Error linking paletteuse main input" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }
    
    ret = avfilter_link(paletteuse_pal_ctx, 0, paletteuse_ctx, 1);
    if (ret < 0) {
        std::cout << "Error linking paletteuse palette input" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }
    
    ret = avfilter_link(paletteuse_ctx, 0, paletteuse_sink_ctx, 0);
    if (ret < 0) {
        std::cout << "Error linking paletteuse output" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 配置paletteuse滤镜图
    ret = avfilter_graph_config(paletteuse_graph, nullptr);
    if (ret < 0) {
        std::cout << "Error configuring paletteuse graph" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // 推送调色板帧
    ret = av_buffersrc_add_frame(paletteuse_pal_ctx, paletteFrame);
    if (ret < 0) {
        std::cout << "Error feeding palette frame" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }
    
    // 关闭调色板输入（发送EOF），表示调色板已完整
    ret = av_buffersrc_add_frame(paletteuse_pal_ctx, nullptr);
    if (ret < 0) {
        std::cout << "Error closing palette input" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }
    
    std::cout << "Palette frame fed to paletteuse filter" << std::endl;

    // ========== 设置输出 ==========
    avformat_alloc_output_context2(&formatContext, nullptr, "gif", storagePath);
    if (!formatContext) {
        std::cout << "Could not create output context" << std::endl;
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_GIF);
    if (!codec) {
        std::cout << "GIF Codec not found" << std::endl;
        avformat_free_context(formatContext);
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    videoStream = avformat_new_stream(formatContext, nullptr);
    if (!videoStream) {
        std::cout << "Could not create video stream" << std::endl;
        avformat_free_context(formatContext);
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    codecContext->width = m_VideoInfo.width;
    codecContext->height = m_VideoInfo.height;
    codecContext->time_base = AVRational{1, m_VideoInfo.frame_rate};
    codecContext->framerate = AVRational{m_VideoInfo.frame_rate, 1};
    codecContext->pix_fmt = AV_PIX_FMT_PAL8;
    codecContext->gop_size = 1;
    codecContext->max_b_frames = 0;

    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0) {
        std::cout << "Could not open codec" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_free_context(formatContext);
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    avcodec_parameters_from_context(videoStream->codecpar, codecContext);
    videoStream->time_base = codecContext->time_base;

    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&formatContext->pb, storagePath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cout << "Could not open output file" << std::endl;
            avcodec_free_context(&codecContext);
            avformat_free_context(formatContext);
            avfilter_graph_free(&paletteuse_graph);
            av_frame_free(&paletteFrame);
            av_frame_free(&rgbFrame);
            sws_freeContext(swsContext);
            return false;
        }
    }

    ret = avformat_write_header(formatContext, nullptr);
    if (ret < 0) {
        std::cout << "Error writing header" << std::endl;
        avcodec_free_context(&codecContext);
        if (!(formatContext->oformat->flags & AVFMT_NOFILE))
            avio_closep(&formatContext->pb);
        avformat_free_context(formatContext);
        avfilter_graph_free(&paletteuse_graph);
        av_frame_free(&paletteFrame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsContext);
        return false;
    }

    // ========== 处理并编码所有帧 ==========
    for (int i = 0; i < m_VideoInfo.raw_image_data.size(); ++i) {
        // RGBA -> RGB24
        uint8_t* inData[1] = { m_VideoInfo.raw_image_data[i].data() };
        int inLinesize[1] = { m_VideoInfo.bytes_per_line };
        
        ret = sws_scale(swsContext, inData, inLinesize, 0, m_VideoInfo.height,
                       rgbFrame->data, rgbFrame->linesize);
        if (ret < 0) {
            std::cout << "Error converting RGBA to RGB24" << std::endl;
            break;
        }

        rgbFrame->pts = i;
        
        // 推送到paletteuse滤镜
        ret = av_buffersrc_add_frame_flags(paletteuse_src_ctx, rgbFrame, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret < 0) {
            std::cout << "Error feeding frame to paletteuse" << std::endl;
            break;
        }

        // 获取PAL8帧
        palFrame = av_frame_alloc();
        ret = av_buffersink_get_frame(paletteuse_sink_ctx, palFrame);
        if (ret < 0) {
            std::cout << "Error getting PAL8 frame" << std::endl;
            av_frame_free(&palFrame);
            break;
        }

        // 编码
        ret = avcodec_send_frame(codecContext, palFrame);
        av_frame_free(&palFrame);
        
        if (ret < 0) {
            std::cout << "Error sending frame to encoder" << std::endl;
            break;
        }

        while (true) {
            ret = avcodec_receive_packet(codecContext, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                std::cout << "Error encoding frame" << std::endl;
                break;
            }

            av_packet_rescale_ts(&pkt, codecContext->time_base, videoStream->time_base);
            pkt.stream_index = videoStream->index;
            av_interleaved_write_frame(formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // 刷新paletteuse滤镜
    av_buffersrc_add_frame_flags(paletteuse_src_ctx, nullptr, 0);
    
    while (true) {
        palFrame = av_frame_alloc();
        ret = av_buffersink_get_frame(paletteuse_sink_ctx, palFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&palFrame);
            break;
        }
        if (ret < 0) {
            av_frame_free(&palFrame);
            break;
        }

        avcodec_send_frame(codecContext, palFrame);
        av_frame_free(&palFrame);

        while (true) {
            ret = avcodec_receive_packet(codecContext, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) break;

            av_packet_rescale_ts(&pkt, codecContext->time_base, videoStream->time_base);
            pkt.stream_index = videoStream->index;
            av_interleaved_write_frame(formatContext, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // 刷新编码器
    avcodec_send_frame(codecContext, nullptr);
    while (true) {
        ret = avcodec_receive_packet(codecContext, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) break;

        av_packet_rescale_ts(&pkt, codecContext->time_base, videoStream->time_base);
        pkt.stream_index = videoStream->index;
        av_interleaved_write_frame(formatContext, &pkt);
        av_packet_unref(&pkt);
    }

    // 写入文件尾
    av_write_trailer(formatContext);

    // 释放资源
    av_frame_free(&rgbFrame);
    av_frame_free(&paletteFrame);
    avfilter_graph_free(&paletteuse_graph);
    sws_freeContext(swsContext);
    avcodec_free_context(&codecContext);
    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
        avio_closep(&formatContext->pb);
    avformat_free_context(formatContext);

    std::cout << "GIF saved successfully!" << std::endl;
    return true;


//    AVFormatContext* formatContext = nullptr;
//    AVCodecContext* codecContext = nullptr;
//    AVStream* videoStream = nullptr;
//    AVFrame* frame = nullptr;
//    AVPacket pkt;
//    av_init_packet(&pkt);  // 初始化 AVPacket
//    pkt.data = nullptr;
//    pkt.size = 0;
//    SwsContext* swsContext;
//    int ret;
//
//    //=====
//
////    // 构建滤镜图
////    AVFilterGraph* filter_graph = avfilter_graph_alloc();
////    if (!filter_graph) {
////        std::cerr << "无法创建滤镜图" << std::endl;
////        return -1;
////    }
////
////    // 创建 buffer source（输入节点），命名为 "in"
////    AVFilterContext* buffersrc_ctx = nullptr;
////    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
////    char args[512];
////    // 构造 buffer 源参数：视频尺寸、像素格式、时间基（这里设为1/25）和像素纵横比
////    snprintf(args, sizeof(args),
////             "video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=1/1",
////             m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGBA);
////    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, nullptr, filter_graph);
////    if (ret < 0) {
////        std::cerr << "创建 buffer source 失败" << std::endl;
////        return -1;
////    }
////
////    // 创建 buffer sink（输出节点），命名为 "out"
////    AVFilterContext* buffersink_ctx = nullptr;
////    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
////    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", nullptr, nullptr, filter_graph);
////    if (ret < 0) {
////        std::cerr << "创建 buffer sink 失败" << std::endl;
////        return -1;
////    }
////    // 目标输出为调色板化后格式，比如 GIF 需要 AV_PIX_FMT_PAL8
////    enum AVPixelFormat out_pix_fmts[] = { AV_PIX_FMT_PAL8, AV_PIX_FMT_NONE };
////    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", out_pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
////    if (ret < 0) {
////        std::cerr << "设置 buffer sink 输出像素格式失败" << std::endl;
////        return -1;
////    }
////
////    // 构造滤镜图描述字符串，注意这里我们仅使用 scale 滤镜进行转换，
////    // 如果你只需要缩放，可使用 "scale=目标宽度:目标高度"；如果要转换像素格式，也可以加 format 过滤器。
////    // 示例中我们设置目标为 640x480（可自行调整）并转换为 PAL8 格式。
////    const char* filter_desc = "scale=1920:1080,format=pal8";
////    // 注意：如果需要其他处理，可修改 filter_desc 字符串。
////
////    // 配置 avfilter_graph_parse_ptr 输入输出
////    AVFilterInOut* outputs = avfilter_inout_alloc();
////    AVFilterInOut* inputs  = avfilter_inout_alloc();
////    if (!outputs || !inputs) {
////        std::cerr << "分配 FilterInOut 失败" << std::endl;
////        return -1;
////    }
////    outputs->name       = av_strdup("in");
////    outputs->filter_ctx = buffersrc_ctx;
////    outputs->pad_idx    = 0;
////    outputs->next       = nullptr;
////
////    inputs->name        = av_strdup("out");
////    inputs->filter_ctx  = buffersink_ctx;
////    inputs->pad_idx     = 0;
////    inputs->next        = nullptr;
////
////    ret = avfilter_graph_parse_ptr(filter_graph, filter_desc, &inputs, &outputs, nullptr);
////    if (ret < 0) {
////        std::cerr << "解析滤镜图失败" << std::endl;
////        return -1;
////    }
////    ret = avfilter_graph_config(filter_graph, nullptr);
////    if (ret < 0) {
////        std::cerr << "配置滤镜图失败" << std::endl;
////        return -1;
////    }
////    avfilter_inout_free(&inputs);
////    avfilter_inout_free(&outputs);
//
//    //=====
//
//    // 创建全局调色板
//    std::vector<uint32_t> GlobalPalette = GenerateGlobalPalette();
//
//    const char* storagePath = m_VideoInfo.output_path.c_str();
//    // 创建输出格式
//    avformat_alloc_output_context2(&formatContext, nullptr, "gif", storagePath);
//    if (!formatContext) {
//        std::cout << "Could not create output context";
//        return false;
//    }
//
//    // 选择编码器（GIF 编码器）
//    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_GIF);
//    if (!codec) {
//        std::cout << "GIF Codec not found";
//        return false;
//    }
//
//    // 创建视频流
////    videoStream = avformat_new_stream(formatContext, nullptr);
//    videoStream = avformat_new_stream(formatContext, codec);
//    if (!videoStream) {
//        std::cout << "Could not create video stream";
//        return false;
//    }
//
//    // 设置编码器参数
//    codecContext = avcodec_alloc_context3(codec);
//    codecContext->width = m_VideoInfo.width;
//    codecContext->height = m_VideoInfo.height;
//    codecContext->gop_size = 1; // 每12帧插入一个I帧
//    codecContext->max_b_frames = 1;
//    codecContext->time_base = AVRational{1, m_VideoInfo.frame_rate};   // 设置时间基为 1/fps 秒
//    codecContext->framerate = AVRational{m_VideoInfo.frame_rate, 1};   // 设置帧率
////    codecContext->pix_fmt = AV_PIX_FMT_RGB8; // 使用RGB格式
//    codecContext->pix_fmt = AV_PIX_FMT_PAL8; // 使用RGB格式
//
//    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
//        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//
//    // 打开编码器
//    ret = avcodec_open2(codecContext, codec, nullptr);
//    if (ret < 0) {
//        std::cout << "Could not open codec";
//        return false;
//    }
//
//    videoStream->codecpar->codec_id = formatContext->oformat->video_codec;
//    videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
//    videoStream->codecpar->width = codecContext->width;
//    videoStream->codecpar->height = codecContext->height;
//    videoStream->codecpar->format = codecContext->pix_fmt;
//    videoStream->time_base = codecContext->time_base;
//    avcodec_parameters_from_context(videoStream->codecpar, codecContext);
//
//    // 打开输出文件
//    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
//        ret = avio_open(&formatContext->pb, storagePath, AVIO_FLAG_WRITE);
//        if (ret < 0) {
//            std::cout << "Could not open output file";
//            return false;
//        }
//    }
//
//    // 写入文件头
//    ret = avformat_write_header(formatContext, nullptr);
//    if (ret < 0) {
//        std::cout << "Error occurred when writing header";
//        return false;
//    }
//
//
//    // 创建SWS上下文，将图像从RGBA转换为GIF所需格式
//    swsContext = sws_getContext(m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGBA,
//                                m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_RGB8,
////                                m_VideoInfo.width, m_VideoInfo.height, AV_PIX_FMT_PAL8, /*pal8 is not supported as output pixel format*/
//                                SWS_BILINEAR, nullptr, nullptr, nullptr);
//
//    // 创建帧
//    frame = av_frame_alloc();
////    frame->format = codecContext->pix_fmt;
//    frame->format = AV_PIX_FMT_RGBA;
//    frame->width = codecContext->width;
//    frame->height = codecContext->height;
//    frame->pict_type = AV_PICTURE_TYPE_NONE;
//    ret = av_frame_get_buffer(frame, 32);
//    if (ret < 0) {
//        std::cout << "Could not allocate frame data";
//        return false;
//    }
//
//    // 循环处理每帧
//    for (int i = 0; i < m_VideoInfo.raw_image_data.size(); ++i) {
//
//
////        for(int y = 0; y < m_VideoInfo.height; y ++){
////            uint8_t* row = frame->data[0] + y * m_VideoInfo.bytes_per_line;
////            memcpy(row, m_VideoInfo.raw_image_data[i].data() + y * m_VideoInfo.bytes_per_line, m_VideoInfo.bytes_per_line);
////        }
////        // 将输入帧推入滤镜图
////        ret = av_buffersrc_add_frame(buffersrc_ctx, frame);
////        if (ret < 0) {
////            std::cerr << "推送帧到滤镜图失败" << std::endl;
////            return -1;
////        }
////        av_buffersink_get_frame(buffersink_ctx, frame);
//
//        // 将全局调色板拷贝到 frame->data[1]
////        memcpy(frame->data[1], GlobalPalette.data(), 256 * sizeof(uint32_t));
//        // 将uint8_t转换为YUV格式
//        uint8_t* inData[1] = { m_VideoInfo.raw_image_data[i].data() };
//        int inLinesize[1] = { m_VideoInfo.bytes_per_line };
//
//        // 转换为GIF格式
//        sws_scale(swsContext, inData, inLinesize, 0, m_VideoInfo.height, frame->data, frame->linesize);
//
//        // 设置帧的 PTS
//        frame->pts = i;
//        // 发送帧到编码器
//        ret = avcodec_send_frame(codecContext, frame);
//        if (ret < 0) {
//            std::cout << "Error sending frame: " << ret << std::endl;
//            break;
//        }
//
//        // 接收编码包
//        while (true) {
//            ret = avcodec_receive_packet(codecContext, &pkt);
//            if (ret == AVERROR(EAGAIN)) {
//                break;
//            } else if (ret == AVERROR_EOF) {
//                break;
//            } else if (ret < 0) {
//                std::cout << "Error encoding frame: " << ret << std::endl;
//                break;
//            }
//
//            // 转换时间基
//            av_packet_rescale_ts(&pkt, codecContext->time_base, videoStream->time_base);
//            pkt.stream_index = videoStream->index;
//
//            std::cout << "Encoded packet size for frame " << i << ": " << pkt.size << std::endl;
//            // 写入文件
//            av_interleaved_write_frame(formatContext, &pkt);
//            av_packet_unref(&pkt);
//        }
//    }
//
//    // 发送 null 帧，确保编码器输出所有剩余的包
//    ret = avcodec_send_frame(codecContext, nullptr);
//    if (ret >= 0) {
//        while (true) {
//            ret = avcodec_receive_packet(codecContext, &pkt);
//            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//                break;
//            if (ret < 0) {
//                std::cout << "Error encoding final frame." << std::endl;
//                break;
//            }
//            pkt.stream_index = videoStream->index;
//            av_interleaved_write_frame(formatContext, &pkt);
//            av_packet_unref(&pkt);
//        }
//    }
//
//    // 写入文件尾
//    av_write_trailer(formatContext);
//
//    // 释放资源
//    avcodec_free_context(&codecContext);
//    av_frame_free(&frame);
//    sws_freeContext(swsContext);
//    if (!(formatContext->oformat->flags & AVFMT_NOFILE))
//        avio_closep(&formatContext->pb);
//    avformat_free_context(formatContext);
//
//    return true;
}

std::vector<uint32_t> FFMPEGVideoWriter::GenerateGlobalPalette() {
    // 使用 unordered_map 统计每个颜色出现的次数
    std::unordered_map<uint32_t, size_t> colorCount;

    for (const auto& img : m_VideoInfo.raw_image_data) {
        // 假设 img.size() == width * height * 4
        int numPixels = m_VideoInfo.width * m_VideoInfo.height;
        for (int i = 0; i < numPixels; ++i) {
            // 取出 RGBA 分量（假设数据顺序为 R, G, B, A）
            uint8_t r = img[i * 4 + 0];
            uint8_t g = img[i * 4 + 1];
            uint8_t b = img[i * 4 + 2];
            // 忽略原始 alpha，统一设为 0xFF
            uint32_t color = (static_cast<uint32_t>(r) << 24) |
                             (static_cast<uint32_t>(g) << 16) |
                             (static_cast<uint32_t>(b) << 8)  |
                             0xFF;
            colorCount[color]++;
        }
    }

    // 将统计结果放入 vector 并按照出现次数从大到小排序
    std::vector<std::pair<uint32_t, size_t>> freq(colorCount.begin(), colorCount.end());
    std::sort(freq.begin(), freq.end(),
              [](const std::pair<uint32_t, size_t>& a, const std::pair<uint32_t, size_t>& b) {
                  return a.second > b.second;
              }
    );

    // 选择出现次数最多的 256 个颜色作为调色板
    std::vector<uint32_t> palette;
    palette.reserve(256);
    for (int i = 0; i < 256; ++i) {
        if (i < static_cast<int>(freq.size()))
            palette.push_back(freq[i].first);
        else
            palette.push_back(0x000000FF); // 如果颜色数不足，则填充黑色
    }
    return palette;

//    std::vector<uint32_t> palette(256, 0);
//
//    // 索引 0～15：常见系统颜色（你可以根据需要修改）
//    uint32_t sysColors[16] = {
//            0x000000FF, // 0: 黑色
//            0x800000FF, // 1: 深红
//            0x008000FF, // 2: 深绿
//            0x808000FF, // 3: 橄榄
//            0x000080FF, // 4: 深蓝
//            0x800080FF, // 5: 紫色
//            0x008080FF, // 6: 深青
//            0xC0C0C0FF, // 7: 浅灰
//            0x808080FF, // 8: 深灰
//            0xFF0000FF, // 9: 红色
//            0x00FF00FF, //10: 绿色
//            0xFFFF00FF, //11: 黄色
//            0x0000FFFF, //12: 蓝色
//            0xFF00FFFF, //13: 品红
//            0x00FFFFFF, //14: 青色
//            0xFFFFFFFF  //15: 白色
//    };
//    for (int i = 0; i < 16; ++i) {
//        palette[i] = sysColors[i];
//    }
//
//    // 索引 16～231：6×6×6 色块
//    int index = 16;
//    int steps[6] = {0, 95, 135, 175, 215, 255};
//    for (int r = 0; r < 6; ++r) {
//        for (int g = 0; g < 6; ++g) {
//            for (int b = 0; b < 6; ++b) {
//                uint8_t R = steps[r];
//                uint8_t G = steps[g];
//                uint8_t B = steps[b];
//                // 组合为 0xRRGGBBAA，Alpha 固定为 0xFF
//                palette[index++] = (static_cast<uint32_t>(R) << 24) |
//                                   (static_cast<uint32_t>(G) << 16) |
//                                   (static_cast<uint32_t>(B) << 8)  |
//                                   0xFF;
//            }
//        }
//    }
//
//    // 索引 232～255：灰度色阶
//    // 起始值为 8，每次增加 10，共 24 个等级
//    for (int i = 232; i < 256; ++i) {
//        uint8_t gray = 8 + (i - 232) * 10;
//        palette[i] = (static_cast<uint32_t>(gray) << 24) |
//                     (static_cast<uint32_t>(gray) << 16) |
//                     (static_cast<uint32_t>(gray) << 8)  |
//                     0xFF;
//    }
//    return palette;

}


IGAME_NAMESPACE_END
#endif