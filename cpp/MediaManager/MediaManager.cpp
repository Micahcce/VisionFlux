#include "MediaManager.h"
#define USE_SDL 0

MediaManager::MediaManager()
    : m_renderCallback(nullptr),
      m_frameQueue(nullptr),
      m_sdlPlayer(nullptr),
      m_pFormatCtx(nullptr),
      m_videoIndex(-1),
      m_audioIndex(-1),
      m_pCodecCtx_video(nullptr),
      m_pCodecCtx_audio(nullptr),
      m_pCodec_video(nullptr),
      m_pCodec_audio(nullptr),
      m_aspectRatio(0.0f),
      m_RGBMode(false),
      m_pAudioParams(nullptr),
      m_swrCtx(nullptr),
      m_frameRGB(nullptr),
      m_pSwsCtx(nullptr),
      m_thread_quit(true),
      m_thread_pause(false),
      m_thread_safe_exited(true),
      m_thread_decode_exited(true),
      m_thread_video_exited(true),
      m_thread_audio_exited(true),
      m_videoLastPTS(0.0),
      m_audioLastPTS(0.0)
{
    m_RGBMode = true;       // 目前仅对SDL有效，Qt只能为RGB渲染
//    logger.setLogLevel(LogLevel::INFO);
    logger.debug("avformat_version :%d", avformat_version());
#if USE_SDL

#endif
}

MediaManager::~MediaManager()
{
    close();
}


AVFormatContext* MediaManager::getMediaInfo(const char* filePath)
{
    //1.创建上下文，注意使用后要释放
    AVFormatContext* formatCtx = avformat_alloc_context();

    //2.打开文件
    int ret = avformat_open_input(&formatCtx, filePath, nullptr, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avformat_open_input");

    //3.上下文获取流信息
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avformat_find_stream_info");

    return formatCtx;
}

void MediaManager::decodeToPlay(const char* filePath)
{
    m_frameQueue = new FrameQueue;
    int ret;

    //1.创建上下文
    m_pFormatCtx = avformat_alloc_context();

    //2.打开文件
    ret = avformat_open_input(&m_pFormatCtx, filePath, nullptr, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avformat_open_input");

    //3.上下文获取流信息
    ret = avformat_find_stream_info(m_pFormatCtx, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avformat_find_stream_info");

    //4.查找视频流和音频流
    m_videoIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(m_videoIndex < 0)
        logger.error("Error occurred in video av_find_best_stream");

    m_audioIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(m_audioIndex < 0)
        logger.error("Error occurred in audio av_find_best_stream");

    //5.获取视频数据
    av_dump_format(m_pFormatCtx, -1, nullptr, 0);

    //6.创建音视频解码器上下文
    m_pCodecCtx_video = avcodec_alloc_context3(nullptr);
    m_pCodecCtx_audio = avcodec_alloc_context3(nullptr);

    //7.解码器上下文获取参数
    ret = avcodec_parameters_to_context(m_pCodecCtx_video, m_pFormatCtx->streams[m_videoIndex]->codecpar);
    if(ret < 0)
        logger.error("Error occurred in avcodec_parameters_to_context");

    ret = avcodec_parameters_to_context(m_pCodecCtx_audio, m_pFormatCtx->streams[m_audioIndex]->codecpar);
    if(ret < 0)
        logger.error("Error occurred in avcodec_parameters_to_context");

    //8.查找解码器
    m_pCodec_video = avcodec_find_decoder(m_pCodecCtx_video->codec_id);
    m_pCodec_audio = avcodec_find_decoder(m_pCodecCtx_audio->codec_id);
    if(!(m_pCodec_video || m_pCodecCtx_audio))
        logger.error("Error occurred in avcodec_find_decoder");

    //9.打开解码器并绑定上下文
    ret = avcodec_open2(m_pCodecCtx_video, m_pCodec_video, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avcodec_open2");

    ret = avcodec_open2(m_pCodecCtx_audio, m_pCodec_audio, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avcodec_open2");

    //10.保存视频宽高比
    m_aspectRatio = static_cast<float>(m_pCodecCtx_video->width) / static_cast<float>(m_pCodecCtx_video->height);

//////////音频//////////
    m_pAudioParams = new AudioParams;
    //1.设置音频播放采样参数
    m_pAudioParams->out_sample_rate = m_pCodecCtx_audio->sample_rate;           //采样率    48000
    m_pAudioParams->out_channels =    m_pCodecCtx_audio->ch_layout.nb_channels; //通道数    2
    m_pAudioParams->out_nb_samples =  m_pCodecCtx_audio->frame_size;            //单个通道中的样本数  1024
    m_pAudioParams->out_sample_fmt =  AV_SAMPLE_FMT_FLT;                        //声音格式  SDL仅支持部分音频格式

    //7.依据参数计算输出缓冲区大小
    m_pAudioParams->out_buffer_size = av_samples_get_buffer_size(NULL, m_pAudioParams->out_channels, m_pAudioParams->out_nb_samples, m_pAudioParams->out_sample_fmt, 1);

    //8.创建音频数据缓冲区
    m_pAudioParams->outBuff = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * m_pAudioParams->out_channels);

    //9.创建重采样上下文
    m_swrCtx = swr_alloc();

    //10.分配重采样的上下文信息
    swr_alloc_set_opts2(&m_swrCtx,
                           &m_pCodecCtx_audio->ch_layout,             /*out*/
                           m_pAudioParams->out_sample_fmt,                    /*out*///fltp->slt
                           m_pAudioParams->out_sample_rate,                   /*out*/
                           &m_pCodecCtx_audio->ch_layout,               /*in*/
                           m_pCodecCtx_audio->sample_fmt,               /*in*/
                           m_pCodecCtx_audio->sample_rate,              /*in*/
                           0,
                           NULL);

    //11.swr上下文初始化
    swr_init(m_swrCtx);

    //开启音频设备
    m_sdlPlayer = new SdlPlayer;
    m_sdlPlayer->initAudioDevice(m_pAudioParams);

//////////创建解码线程//////////
    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;
    m_thread_quit = false;
    m_thread_pause = false;
    m_thread_safe_exited = false;
    m_thread_decode_exited = false;
    m_thread_video_exited = false;
    m_thread_audio_exited = false;

    if(m_RGBMode)
        frameYuvToRgb();

#if USE_SDL
    m_sdlPlayer->initVideoDevice(m_pCodecCtx_video->width, m_pCodecCtx_video->height, m_RGBMode);

    SDL_CreateThread(decodeThreadEntry, NULL, this);
    SDL_CreateThread(videoThreadEntry, NULL, this);
    SDL_CreateThread(audioThreadEntry, NULL, this);

    SDL_Event event;                    //定义事件
    while(true)
    {
        SDL_WaitEvent(&event);

        if(event.type == SDL_QUIT)      //程序退出
        {
            this->setThreadQuit(true);
            break;
        }
        else if(event.type == SDL_KEYDOWN)
        {
            if(event.key.keysym.sym == SDLK_SPACE)  //空格键暂停
            {
                this->setThreadPause(!this->m_thread_pause);
            }
        }
        else if(event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                // 重新初始化渲染资源
                this->getSdlPlayer()->resize(event.window.data1, event.window.data2, m_RGBMode);
            }
        }
    }
#else
    std::thread m_decodeThread(&MediaManager::thread_media_decode, this);
    std::thread m_videoThread(&MediaManager::thread_video_display, this);
    std::thread m_audioThread(&MediaManager::thread_audio_display, this);
    m_decodeThread.detach();
    m_videoThread.detach();
    m_audioThread.detach();
#endif
}

void MediaManager::seekMedia(int timeSecs)
{
    m_frameQueue->clear();

    //根据视频帧来seek，因为音频帧的解码不需要I帧，跳转后可能会影响到视频帧的解码
    AVRational time_base = m_pFormatCtx->streams[m_videoIndex]->time_base;
    logger.debug("time_base.num: %d", time_base.num);
    logger.debug("time_base.den: %d", time_base.den);

    // 将目标时间转换为PTS
    int64_t targetPTS = av_rescale_q(timeSecs * AV_TIME_BASE, AV_TIME_BASE_Q, time_base); // 将时间转换为PTS
    logger.info("seek PTS: %d", targetPTS);

    // 使用 AVSEEK_FLAG_BACKWARD 来确保向前查找最近的 I 帧
    if (av_seek_frame(m_pFormatCtx, m_videoIndex, targetPTS, AVSEEK_FLAG_BACKWARD) < 0)
    {
        logger.error("Error seeking to position.");
        return;
    }
    avcodec_flush_buffers(m_pCodecCtx_video);
    avcodec_flush_buffers(m_pCodecCtx_audio);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool throwing = true;

    //丢弃多余帧
    while (throwing)
    {
        if (av_read_frame(m_pFormatCtx, packet) < 0)
            break; // 没有更多的帧

        if (packet->stream_index == m_videoIndex)
        {
            avcodec_send_packet(m_pCodecCtx_video, packet);
            while (avcodec_receive_frame(m_pCodecCtx_video, frame) >= 0)
            {
                if (frame->pts >= targetPTS)
                {
                    // 找到目标帧
                    throwing = false;
                    break;
                }
            }
        }
        av_packet_unref(packet);
    }
    av_frame_free(&frame);
    av_packet_free(&packet);

    logger.info("seek complete");
}


//视频解码线程
int MediaManager::thread_media_decode()
{
    int ret;

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    //解码
    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        if(av_read_frame(m_pFormatCtx, packet) < 0)
            break;

        if(packet->stream_index == m_videoIndex)
        {
            avcodec_send_packet(m_pCodecCtx_video, packet);

            while(1)
            {
                ret = avcodec_receive_frame(m_pCodecCtx_video, frame);

                if(ret < 0)
                {
                    if(ret == AVERROR_EOF)
                        logger.info("Media playback finished.");  // 视频解码结束
                    break;
                }

                while(m_frameQueue->getVideoFrameCount() >= MAX_NODE_NUMBER && !m_thread_quit)
                    delayMs(10);

                m_frameQueue->pushVideoFrame(frame);

                av_frame_unref(frame);
            }
        }
        else if(packet->stream_index == m_audioIndex)
        {
            avcodec_send_packet(m_pCodecCtx_audio, packet);

            while(1)
            {
                ret = avcodec_receive_frame(m_pCodecCtx_audio, frame);

                if(ret < 0)
                {
                    if(ret == AVERROR_EOF)
                        logger.info("Media playback finished.");  // 音频解码结束
                    break;
                }

                while(m_frameQueue->getAudioFrameCount() >= MAX_NODE_NUMBER && !m_thread_quit)
                    delayMs(10);

                m_frameQueue->pushAudioFrame(frame);

                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    m_thread_decode_exited = true;

    return 0;
}

void MediaManager::delayMs(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void MediaManager::close()
{
    logger.debug("wait.");
    m_thread_quit = true;
    if(m_frameQueue)
        m_frameQueue->signalExit();

    //安全退出
    while(m_thread_decode_exited == false || m_thread_video_exited == false || m_thread_audio_exited == false)
    {
        logger.debug("waitting thread exit.");
        delayMs(20);
    }

    //清理资源，注意顺序避免崩溃
    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;

    if (m_pSwsCtx)
    {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = nullptr;
    }

    if (m_sdlPlayer)
    {
        delete m_sdlPlayer;
        m_sdlPlayer = nullptr;
    }

    if (m_pAudioParams && m_pAudioParams->outBuff)
    {
        av_free(m_pAudioParams->outBuff);
        m_pAudioParams->outBuff = nullptr;
    }

    if (m_frameQueue)
    {
        delete m_frameQueue;
        m_frameQueue = nullptr;
    }

    if(m_swrCtx)
    {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }

    if(m_pCodecCtx_video)
    {
        avcodec_free_context(&m_pCodecCtx_video);
        m_pCodecCtx_video = nullptr;
    }

    if(m_pCodecCtx_audio)
    {
        avcodec_free_context(&m_pCodecCtx_audio);
        m_pCodecCtx_audio = nullptr;
    }

    if(m_pFormatCtx)
    {
        avformat_close_input(&m_pFormatCtx);
        m_pFormatCtx = nullptr;
    }

    //安全退出标志
    m_thread_safe_exited = true;
    logger.info("all thread exit.");
}

//视频播放线程
int MediaManager::thread_video_display()
{
    //渲染
    AVFrame* frame = av_frame_alloc();

    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        frame = m_frameQueue->popVideoFrame();
        if(!frame)
            continue;

        //渲染
#if USE_SDL
    if(m_RGBMode)
    {
        sws_scale(m_pSwsCtx,
                  (const unsigned char* const*)frame->data,
                  frame->linesize, 0,
                  m_pCodecCtx_video->height,
                  m_frameRGB->data,
                  m_frameRGB->linesize);
        m_sdlPlayer->renderFrameRGB(m_frameRGB);
    }
    else
        m_sdlPlayer->renderFrame(frame);
#else
        sws_scale(m_pSwsCtx,
                  (const unsigned char* const*)frame->data,
                  frame->linesize, 0,
                  m_pCodecCtx_video->height,
                  m_frameRGB->data,
                  m_frameRGB->linesize);

        // 调用回调函数，通知 GUI 渲染
        if (m_renderCallback)
            m_renderCallback(m_frameRGB, m_pCodecCtx_video->width, m_pCodecCtx_video->height, m_aspectRatio);
        else
            logger.error("Render callback not set");
#endif

        //延时控制
        videoDelayControl(frame);

        av_frame_unref(frame);
    }
    av_frame_free(&frame);
    m_thread_video_exited = true;

    return 0;
}


//音频播放线程
int MediaManager::thread_audio_display()
{
    int ret;

    //渲染
    AVFrame* frame = av_frame_alloc();

    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        frame = m_frameQueue->popAudioFrame();
        if(!frame)
            continue;

        ret = swr_convert(m_swrCtx, &m_pAudioParams->outBuff, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
        if(ret < 0)
        {
            logger.error("Error while converting\n");
            break;
        }

        while(m_sdlPlayer->m_audioLen > 0)
            delayMs(1);

        audioDelayControl(frame);

        //音频填充参数
        m_sdlPlayer->m_audioChunk = (unsigned char *)m_pAudioParams->outBuff;
        m_sdlPlayer->m_audioPos = m_sdlPlayer->m_audioChunk;
        m_sdlPlayer->m_audioLen = m_pAudioParams->out_buffer_size;

        av_frame_unref(frame);
    }
    av_frame_free(&frame);
    m_thread_audio_exited = true;

    return 0;
}

void MediaManager::videoDelayControl(AVFrame* frame)
{
    double currentVideoPTS = frame->pts * av_q2d(m_pFormatCtx->streams[m_videoIndex]->time_base);

    // 获取当前音频PTS
    double currentAudioPTS = m_audioLastPTS;

    // 判断视频是否超前于音频，如果超前则等待
    if (currentVideoPTS > currentAudioPTS)
    {
        double delayDuration = currentVideoPTS - currentAudioPTS;
        if (delayDuration > 0.0 && m_thread_quit == false)
        {
            av_usleep(delayDuration * 1000000); // 延时
        }
    }
//    logger.debug("Current Video PTS: %f, Last PTS: %f", currentVideoPTS, m_videoLastPTS);

    // 记录当前视频PTS
    m_videoLastPTS = currentVideoPTS;
}

void MediaManager::audioDelayControl(AVFrame *frame)
{
    //音频按pts渲染
    double currentAudioPTS = frame->pts * av_q2d(m_pFormatCtx->streams[m_audioIndex]->time_base);
    if (m_audioLastPTS != 0.0)
    {
        double delayDuration = currentAudioPTS - m_audioLastPTS;
        if (delayDuration > 0.0 && delayDuration < AV_TIME_BASE && m_thread_quit == false)
        {
            av_usleep(delayDuration); // 微秒延时
        }
    }
//    logger.debug("Current Audio PTS: %f, Last PTS: %f", currentAudioPTS, m_audioLastPTS);

    // 记录当前音频PTS
    m_audioLastPTS = currentAudioPTS;
}

void MediaManager::frameYuvToRgb()
{   
    int srcWidth = m_pCodecCtx_video->width;
    int srcHeight = m_pCodecCtx_video->height;

#if USE_SDL
    int dstWidth = srcWidth;
    int dstHeight = srcHeight;
#else
    int dstWidth = srcWidth;
    int dstHeight = srcHeight;

#endif

    m_frameRGB = av_frame_alloc();                   //转换后的帧
    m_pSwsCtx = sws_getContext(srcWidth, srcHeight, m_pCodecCtx_video->pix_fmt, dstWidth, dstHeight, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    unsigned char *buf = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, dstWidth*2, dstHeight*2, 1));
    av_image_fill_arrays(m_frameRGB->data, m_frameRGB->linesize, buf, AV_PIX_FMT_RGB32, dstWidth, dstHeight, 1);
}




