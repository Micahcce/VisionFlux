#include "MediaManager.h"
#define USE_SDL 0

using namespace soundtouch;

MediaManager::MediaManager()
    : m_renderCallback(nullptr),
      m_frameQueue(nullptr),
      m_sdlPlayer(nullptr),
      m_soundTouch(nullptr),
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
      m_audioLastPTS(0.0),
      m_speedFactor(1.0)
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


AVFormatContext* MediaManager::getMediaInfo(const std::string& filePath)
{
    //1.创建上下文，注意使用后要释放
    AVFormatContext* formatCtx = avformat_alloc_context();

    //2.打开文件
    int ret = avformat_open_input(&formatCtx, filePath.data(), nullptr, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_open_input");
        avformat_close_input(&m_pFormatCtx);
        return nullptr;
    }

    //3.上下文获取流信息
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_find_stream_info");
        avformat_close_input(&m_pFormatCtx);
        return nullptr;
    }

    return formatCtx;
}

bool MediaManager::decodeToPlay(const std::string& filePath)
{
    m_frameQueue = new FrameQueue;
    int ret;

    //1.创建上下文
    m_pFormatCtx = avformat_alloc_context();

    //2.打开文件
    ret = avformat_open_input(&m_pFormatCtx, filePath.data(), nullptr, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_open_input");
        avformat_close_input(&m_pFormatCtx);
        return false;
    }

    //3.上下文获取流信息
    ret = avformat_find_stream_info(m_pFormatCtx, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_find_stream_info");
        avformat_close_input(&m_pFormatCtx);
        return false;
    }

    //4.查找视频流和音频流
    m_videoIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(m_videoIndex < 0)
        logger.warning("Not found video stream");

    m_audioIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(m_audioIndex < 0)
        logger.warning("Not found audio stream");

    //5.获取视频数据
    av_dump_format(m_pFormatCtx, -1, nullptr, 0);


    // 相关变量初始化
    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;
    m_thread_quit = false;
    m_thread_pause = false;
    m_thread_safe_exited = false;
    m_thread_decode_exited = false;
    m_thread_video_exited = false;
    m_thread_audio_exited = false;

    if(m_videoIndex < 0)
        m_thread_video_exited = true;
    if(m_audioIndex < 0)
        m_thread_audio_exited = true;

    // 视频流
    if(m_videoIndex >= 0)
    {
        initVideoCodec();

        m_aspectRatio = static_cast<float>(m_pCodecCtx_video->width) / static_cast<float>(m_pCodecCtx_video->height);

        if(m_RGBMode)
            frameYuvToRgb();

        std::thread videoThread(&MediaManager::thread_video_display, this);
        videoThread.detach();
    }

    // 音频流
    if(m_audioIndex >= 0)
    {
        initAudioCodec();
        initAudioDevice();

        std::thread audioThread(&MediaManager::thread_audio_display, this);
        audioThread.detach();
    }

    //解码线程
    std::thread decodeThread(&MediaManager::thread_media_decode, this);
    decodeThread.detach();


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
#endif
    return true;
}

bool MediaManager::pushStream(const std::string &filePath, const std::string &streamUrl)
{
    m_filePath = filePath;
    m_streamUrl = streamUrl;
    std::thread pushStreamThread(&MediaManager::thread_push_stream, this);
    pushStreamThread.detach();
    return true;
}

void MediaManager::seekFrameByVideoStream(int timeSecs)
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
    if(m_audioIndex >= 0)
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

void MediaManager::seekFrameByAudioStream(int timeSecs)
{
    m_frameQueue->clear();

    AVRational time_base = m_pFormatCtx->streams[m_audioIndex]->time_base;
    logger.debug("time_base.num: %d", time_base.num);
    logger.debug("time_base.den: %d", time_base.den);

    // 将目标时间转换为PTS
    int64_t targetPTS = av_rescale_q(timeSecs * AV_TIME_BASE, AV_TIME_BASE_Q, time_base); // 将时间转换为PTS
    logger.info("seek PTS: %d", targetPTS);

    // 使用 AVSEEK_FLAG_BACKWARD 从当前位置向后查找最近的同步点，不使用该标志则会向前查找
    if (av_seek_frame(m_pFormatCtx, m_audioIndex, targetPTS, AVSEEK_FLAG_BACKWARD) < 0)
    {
        logger.error("Error seeking to position.");
        return;
    }

    if(m_videoIndex >= 0)
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

        if (packet->stream_index == m_audioIndex)
        {
            avcodec_send_packet(m_pCodecCtx_audio, packet);
            while (avcodec_receive_frame(m_pCodecCtx_audio, frame) >= 0)
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

float MediaManager::getCurrentProgress()
{
    if(m_audioIndex >= 0)
        return m_audioLastPTS;
    else
        return m_videoLastPTS;
}

void MediaManager::audioChangeSpeed(float speedFactor)
{
    logger.info("change speed to %0.2f", speedFactor);

    m_soundTouch->setTempo(speedFactor);
    m_sdlPlayer->audioChangeSpeed(speedFactor);
    m_speedFactor = speedFactor;
}

bool MediaManager::saveFrameToBmp(const std::string filePath, const std::string outputPath, int sec)
{
    AVFormatContext* formatCtx = getMediaInfo(filePath);

    int videoStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamIndex == -1) {
        logger.error("Could not find a video stream.");
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodecParameters* codecpar = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        logger.error("Could not find decoder for codec ID: %d", codecpar->codec_id);
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        logger.error("Could not allocate codec context.");
        avformat_close_input(&formatCtx);
        return false;
    }

    if (avcodec_parameters_to_context(codecCtx, codecpar) < 0) {
        logger.error("Could not copy codec parameters to codec context.");
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        logger.error("Could not open codec.");
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* frameRGB = av_frame_alloc();

    //跳转
    if (av_seek_frame(formatCtx, -1, sec, AVSEEK_FLAG_BACKWARD) < 0)
    {
        logger.error("Could not seek to frame, use current position frame.");
    }

    while (av_read_frame(formatCtx, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex)
        {
            if (avcodec_send_packet(codecCtx, packet) >= 0)
            {
                int ret = avcodec_receive_frame(codecCtx, frame);
                if (ret == AVERROR(EAGAIN))
                    continue;
                else if (ret < 0)
                {
                    logger.error("Could not receive a frame");
                    av_frame_free(&frameRGB);
                    av_frame_free(&frame);
                    av_packet_free(&packet);
                    avcodec_free_context(&codecCtx);
                    avformat_close_input(&formatCtx);
                    return false;
                }
                else
                    break;  //获取到了frame直接返回
            }
        }
        av_packet_unref(packet);  // 释放数据包
    }

    // 设置输出图像的参数
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, codecCtx->width, codecCtx->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB32, codecCtx->width, codecCtx->height, 1);

    // 初始化swsCtx（假设codecCtx的参数在整个过程中保持不变）
    SwsContext* swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB32,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsCtx)
    {
        logger.error("Failed to initialize sws context.");
        av_freep(&buffer);
        av_frame_free(&frameRGB);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    // 使用已经初始化的swsCtx进行转换
    sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, frameRGB->data, frameRGB->linesize);

    // 保存为图片
    bool saved = saveBMP(outputPath.data(), buffer, codecCtx->width, codecCtx->height, StorageFormat::BGRA);

    // 清理资源
    sws_freeContext(swsCtx);
    av_freep(&buffer);
    av_frame_free(&frame);
    av_frame_free(&frameRGB);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);

    if(saved)
    {
        logger.info("Image saved successfully at: %s", outputPath.data());
        return true;
    }
    else
    {
        logger.info("Image saved failed at: %s", outputPath.data());
        return false;
    }
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

    if(m_soundTouch)
    {
        delete m_soundTouch;
        m_soundTouch = nullptr;
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

    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;
    m_videoIndex = -1;
    m_audioIndex = -1;

    //安全退出标志
    m_thread_safe_exited = true;
    logger.info("all thread exit.");
}

void MediaManager::initVideoCodec()
{
    int ret = 0;
    //创建音视频解码器上下文
    m_pCodecCtx_video = avcodec_alloc_context3(nullptr);
    //解码器上下文获取参数
    ret = avcodec_parameters_to_context(m_pCodecCtx_video, m_pFormatCtx->streams[m_videoIndex]->codecpar);
    if(ret < 0)
        logger.error("Error occurred in avcodec_parameters_to_context");
    //查找解码器
    m_pCodec_video = avcodec_find_decoder(m_pCodecCtx_video->codec_id);
    if(!m_pCodec_video)
        logger.error("Error occurred in avcodec_find_decoder");
    //打开解码器并绑定上下文
    ret = avcodec_open2(m_pCodecCtx_video, m_pCodec_video, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avcodec_open2");
}

void MediaManager::initAudioCodec()
{
    int ret = 0;
    m_pCodecCtx_audio = avcodec_alloc_context3(nullptr);

    ret = avcodec_parameters_to_context(m_pCodecCtx_audio, m_pFormatCtx->streams[m_audioIndex]->codecpar);
    if(ret < 0)
        logger.error("Error occurred in avcodec_parameters_to_context");

    m_pCodec_audio = avcodec_find_decoder(m_pCodecCtx_audio->codec_id);
    if(!m_pCodec_audio)
        logger.error("Error occurred in avcodec_find_decoder");

    ret = avcodec_open2(m_pCodecCtx_audio, m_pCodec_audio, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avcodec_open2");
}

void MediaManager::initAudioDevice()
{
    ///////音频设备初始化///////
    m_pAudioParams = new AudioParams;
    //设置音频播放采样参数
    m_pAudioParams->out_sample_rate = m_pCodecCtx_audio->sample_rate;           //采样率    48000
    m_pAudioParams->out_channels =    m_pCodecCtx_audio->ch_layout.nb_channels; //通道数    2
    m_pAudioParams->out_nb_samples =  m_pCodecCtx_audio->frame_size;            //单个通道中的样本数  1024
    m_pAudioParams->out_sample_fmt =  AV_SAMPLE_FMT_FLT;                        //声音格式  SDL仅支持部分音频格式

    //创建音频数据缓冲区
    m_pAudioParams->outBuff = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * m_pAudioParams->out_channels);

    //创建重采样上下文
    m_swrCtx = swr_alloc();

    //分配重采样的上下文信息
    swr_alloc_set_opts2(&m_swrCtx,
                           &m_pCodecCtx_audio->ch_layout,             /*out*/
                           m_pAudioParams->out_sample_fmt,                    /*out*///fltp->slt
                           m_pAudioParams->out_sample_rate,                   /*out*/
                           &m_pCodecCtx_audio->ch_layout,               /*in*/
                           m_pCodecCtx_audio->sample_fmt,               /*in*/
                           m_pCodecCtx_audio->sample_rate,              /*in*/
                           0,
                           NULL);

    //swr上下文初始化
    swr_init(m_swrCtx);

    // 初始化 SoundTouch 实例
    m_soundTouch = new SoundTouch;
    m_soundTouch->setSampleRate(m_pAudioParams->out_sample_rate);  // 设置采样率
    m_soundTouch->setChannels(m_pAudioParams->out_channels);      // 设置通道数
    m_soundTouch->setTempo(m_speedFactor);                       // 设置倍速播放

    //开启音频设备
    m_sdlPlayer = new SdlPlayer;
    m_sdlPlayer->initAudioDevice(m_pAudioParams);
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
            m_renderCallback(m_frameRGB->data[0], m_pCodecCtx_video->width, m_pCodecCtx_video->height, m_aspectRatio);
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

        // 将重采样后的音频数据传递给 SoundTouch 进行处理
        m_soundTouch->putSamples((const float*)m_pAudioParams->outBuff, ret);

        // 获取处理后的样本
        int numSamplesProcessed = m_soundTouch->receiveSamples((float*)m_pAudioParams->outBuff, m_pAudioParams->out_nb_samples / m_speedFactor);
//        logger.debug("putSamples = %d, numSamplesProcessed = %d", ret, numSamplesProcessed);

        // 音频PTS计算
        m_audioLastPTS = frame->pts * av_q2d(m_pFormatCtx->streams[m_audioIndex]->time_base);

        // 等待SDL音频播放器完成当前的音频数据处理和输出
        while(m_sdlPlayer->m_audioLen > 0)
            delayMs(1);

        // 音频填充参数
        m_sdlPlayer->m_audioChunk = (unsigned char *)m_pAudioParams->outBuff;
        m_sdlPlayer->m_audioPos = m_sdlPlayer->m_audioChunk;
        m_sdlPlayer->m_audioLen = numSamplesProcessed * m_pAudioParams->out_channels * sizeof(float);  // 处理后的数据长度

        av_frame_unref(frame);
    }
    av_frame_free(&frame);
    m_thread_audio_exited = true;

    return 0;
}

int MediaManager::thread_push_stream()
{
    //1.定义输入输出格式上下文
    AVFormatContext* inputFormatCtx = NULL;
    AVFormatContext* outputFormatCtx = NULL;

    //2.打开输入文件
    int ret = avformat_open_input(&inputFormatCtx, m_filePath.data(), NULL, NULL);
    if(ret < 0) return false;

    //3.打开输出文件
    ret = avformat_alloc_output_context2(&outputFormatCtx, NULL, "flv", m_streamUrl.data());
    if(ret < 0) return false;

    //4.分析流信息
    ret = avformat_find_stream_info(inputFormatCtx, NULL);
    if(ret < 0) return false;

    av_dump_format(inputFormatCtx, 0, m_filePath.data(), 0);            //打印输入信息

    //5.获取流信息
    for(unsigned int i = 0; i < inputFormatCtx->nb_streams; i++)
    {
        AVStream *in_stream = inputFormatCtx->streams[i];
        AVStream *out_stream = avformat_new_stream(outputFormatCtx, NULL);
        if(!out_stream) return -1;

        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if(ret < 0) return false;

        out_stream->codecpar->codec_tag = 0;
    }

    //6.输出封装处理
    if(outputFormatCtx && !(outputFormatCtx->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&outputFormatCtx->pb, m_streamUrl.data(), AVIO_FLAG_WRITE);
        if(ret < 0) return false;
    }

    //7.写文件头
    ret = avformat_write_header(outputFormatCtx, NULL);
    if(ret < 0) return false;

    av_dump_format(outputFormatCtx, 0, m_streamUrl.data(), 1);          //打印输出信息

    //8.定义变量
    uint64_t frame_index = 0;
    AVPacket *pkt = av_packet_alloc();

    AVMediaType media_type;

    int64_t start_time = av_gettime();
    AVRational time_base_q = {1,AV_TIME_BASE}; // us

    bool running = true;

    while(running)
    {
        //9.读取一个包
        if(av_read_frame(inputFormatCtx, pkt) < 0)
            break;

        //10.转换包
        AVStream *in_stream = inputFormatCtx->streams[pkt->stream_index];
        AVStream *out_stream = outputFormatCtx->streams[pkt->stream_index];

        media_type = in_stream->codecpar->codec_type;

        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);     //输入、输出都是rtmp时，时基一样，可省略时间戳转换

        //输出到屏幕
        if(media_type == AVMEDIA_TYPE_VIDEO)
        {
            printf("save frame: %llu , time: %lld\n",frame_index, av_gettime() / 1000); // ms
            frame_index++;
        }

        //11.写包
        ret = av_interleaved_write_frame(outputFormatCtx, pkt);
        if(ret < 0) return ret;

        //适当延时
        delayMs(5);


        av_packet_unref(pkt);
    }

    //12.写文件尾
    ret = av_write_trailer(outputFormatCtx);
    if(ret < 0) return ret;

    //13.关闭输入
    avformat_close_input(&inputFormatCtx);
    av_packet_free(&pkt);

    if(outputFormatCtx && !(outputFormatCtx->flags & AVFMT_NOFILE))
    {
        avio_close(outputFormatCtx->pb);
    }
    avformat_free_context(outputFormatCtx);

    return 0;
}

void MediaManager::videoDelayControl(AVFrame* frame)
{
    double currentVideoPTS = frame->pts * av_q2d(m_pFormatCtx->streams[m_videoIndex]->time_base);

    //有音频时向音频流同步，无音频流则按PTS播放
    if(m_audioIndex >= 0)
    {
        // 判断视频是否超前于音频，如果超前则等待
        if (currentVideoPTS > m_audioLastPTS && m_audioLastPTS != 0.0)
        {
            double delayDuration = currentVideoPTS - m_audioLastPTS;
            if (delayDuration > 0.0 && m_thread_quit == false)
            {
                av_usleep(delayDuration * AV_TIME_BASE);   // 微秒延时
            }
        }
    }
    else
    {
        //视频按pts渲染
        if (m_videoLastPTS != 0.0)
        {
            double delayDuration = currentVideoPTS - m_videoLastPTS;
            if (delayDuration > 0.0 && m_thread_quit == false)
            {
                av_usleep(delayDuration * AV_TIME_BASE); // 微秒延时
            }
        }
    }
//    logger.debug("Current Video PTS: %f, Last PTS: %f, m_audioLastPTS: %f", currentVideoPTS, m_videoLastPTS, m_audioLastPTS);

    // 记录当前视频PTS
    m_videoLastPTS = currentVideoPTS;
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




