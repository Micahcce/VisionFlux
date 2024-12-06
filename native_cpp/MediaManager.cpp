#include "MediaManager.h"


MediaManager::MediaManager()
    : m_renderCallback(nullptr),
      m_mediaQueue(nullptr),
      m_systemClock(nullptr),
      m_sdlPlayer(nullptr),
      m_soundTouch(nullptr),
      m_formatCtx(nullptr),
      m_videoIndex(-1),
      m_audioIndex(-1),
      m_videoCodecCtx(nullptr),
      m_audioCodecCtx(nullptr),
      m_videoCodec(nullptr),
      m_audioCodec(nullptr),
      m_rgbMode(false),
      m_aspectRatio(1.0),
      m_windowWidth(0),
      m_windowHeight(0),
      m_cudaAccelerate(false),
      m_safeCudaAccelerate(false),
      m_deviceCtx(nullptr),
      m_frameBuf(nullptr),
      m_outBuf(nullptr),
      m_swrCtx(nullptr),
      m_frame(nullptr),
      m_frameSw(nullptr),
      m_frameRgb(nullptr),
      m_swsCtx(nullptr),
      m_thread_quit(true),
      m_thread_pause(false),
      m_thread_safe_exited(true),
      m_thread_media_read_exited(true),
      m_thread_video_decode_exited(true),
      m_thread_audio_decode_exited(true),
      m_thread_video_display_exited(true),
      m_thread_audio_display_exited(true),
      m_videoLastPTS(0.0),
      m_audioLastPTS(0.0),
      m_speedFactor(1.0)
{
//    av_log_set_level(AV_LOG_DEBUG);
//    logger.setLogLevel(LogLevel::INFO);
    logger.debug("avformat_version :%d", avformat_version());

    m_rgbMode = true;       // 目前仅对SDL有效，Qt只能为RGB渲染
    m_mediaQueue = new MediaQueue;
    m_systemClock = new SystemClock;
}

MediaManager::~MediaManager()
{
    close();
    delete m_systemClock;
}


bool MediaManager::decodeToPlay(const std::string& filePath)
{
    int ret;

    //1.创建上下文
    m_formatCtx = avformat_alloc_context();

    //2.打开文件
    ret = avformat_open_input(&m_formatCtx, filePath.data(), nullptr, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_open_input");
        avformat_close_input(&m_formatCtx);
        return false;
    }

    //3.上下文获取流信息
    ret = avformat_find_stream_info(m_formatCtx, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_find_stream_info");
        avformat_close_input(&m_formatCtx);
        return false;
    }

    //4.查找视频流和音频流
    m_videoIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(m_videoIndex < 0)
        logger.warning("Not found video stream");

    m_audioIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(m_audioIndex < 0)
        logger.warning("Not found audio stream");

    //5.获取视频数据
    av_dump_format(m_formatCtx, -1, nullptr, 0);


    // 相关变量初始化
    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;
    m_thread_quit = false;
    m_thread_pause = false;
    m_thread_safe_exited = false;
    m_thread_media_read_exited = false;
    m_mediaQueue->reset();

    if(m_videoIndex < 0)
        m_thread_video_display_exited = true;
    if(m_audioIndex < 0)
        m_thread_audio_display_exited = true;

    // 视频流
    if(m_videoIndex >= 0)
    {
#ifdef CUDA_ISAVAILABLE
        m_cudaAccelerate = m_safeCudaAccelerate;
#endif

        initVideoCodec();

        m_aspectRatio = static_cast<double>(m_videoCodecCtx->width) / static_cast<double>(m_videoCodecCtx->height);
        m_windowWidth = m_videoCodecCtx->width;
        m_windowHeight = m_videoCodecCtx->height;
        m_frame = av_frame_alloc();
        m_frameRgb = av_frame_alloc();

        if(m_cudaAccelerate)
        {
            //存放转换后的帧数据
            m_frameSw = av_frame_alloc();
            m_frameSw->format = AV_PIX_FMT_NV12;        //cuda加速后默认输出NV12格式
            m_frameSw->width = m_videoCodecCtx->width;
            m_frameSw->height = m_videoCodecCtx->height;
            av_frame_get_buffer(m_frameSw, 0); // 申请缓冲区
        }

        if(m_rgbMode)
            frameYuvToRgb();
//            frameResize(m_windowWidth, m_windowHeight, true);

        m_thread_video_decode_exited = false;
        m_thread_video_display_exited = false;
        std::thread videoDecodeThread(&MediaManager::thread_video_decode, this);
        videoDecodeThread.detach();
        std::thread videoDisplayThread(&MediaManager::thread_video_display, this);
        videoDisplayThread.detach();
    }

    // 音频流
    if(m_audioIndex >= 0)
    {
        initAudioCodec();
        initAudioDevice();

        m_thread_audio_decode_exited = false;
        m_thread_audio_display_exited = false;
        std::thread audioDecodeThread(&MediaManager::thread_audio_decode, this);
        audioDecodeThread.detach();
        std::thread audioDisplayThread(&MediaManager::thread_audio_display, this);
        audioDisplayThread.detach();
    }

    // 读取线程
    std::thread mediaReadThread(&MediaManager::thread_media_read, this);
    mediaReadThread.detach();

    // 开启系统设置，目前只用于单视频流的渲染延时控制
    m_systemClock->start();


    return true;
}

bool MediaManager::streamConvert(const std::string& inputStreamUrl, const std::string& outputStreamUrl)
{
    m_inputStreamUrl = inputStreamUrl;
    m_outputStreamUrl = outputStreamUrl;
    std::thread streamConvert(&MediaManager::thread_stream_convert, this);
    streamConvert.detach();
    return true;
}

void MediaManager::seekFrameByStream(int timeSecs)
{
    // 优先根据视频帧来seek，因为音频帧的解码不需要I帧，跳转后可能会影响到视频帧的解码
    bool hasVideoStream = (m_videoIndex >= 0) ? true : false;
    int streamIndex = hasVideoStream ? m_videoIndex : m_audioIndex;
    AVCodecContext* codecCtx = hasVideoStream ? m_videoCodecCtx : m_audioCodecCtx;

    AVRational time_base = m_formatCtx->streams[streamIndex]->time_base;
    logger.debug("time_base: %d / %d", time_base.num, time_base.den);

    // 将目标时间转换为PTS
    int64_t targetPTS = av_rescale_q(timeSecs * AV_TIME_BASE, AV_TIME_BASE_Q, time_base);
    logger.info("seek PTS: %lld", targetPTS);

    // 使用 AVSEEK_FLAG_BACKWARD 来确保向前查找最近的 I 帧
    if (av_seek_frame(m_formatCtx, streamIndex, targetPTS, AVSEEK_FLAG_BACKWARD) < 0)
    {
        logger.error("Error seeking to position.");
        return;
    }

    // 清除帧队列
    m_mediaQueue->clear();

    // 清除解码器缓存
    std::unique_lock<std::mutex> lock(m_decodeMtx);
    if(m_videoIndex >= 0)
        avcodec_flush_buffers(m_videoCodecCtx);
    if(m_audioIndex >= 0)
        avcodec_flush_buffers(m_audioCodecCtx);
    lock.unlock();

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool throwing = true;

    // 丢弃多余帧
    while (throwing)
    {
        if (av_read_frame(m_formatCtx, packet) < 0)
            break;

        if (packet->stream_index == streamIndex)
        {
            std::unique_lock<std::mutex> lock(m_decodeMtx);
            avcodec_send_packet(codecCtx, packet);
            lock.unlock();

            while (avcodec_receive_frame(codecCtx, frame) >= 0)
            {
//                logger.debug("throw frame type: %d ,pts: %d", frame->pict_type, frame->pts);
                // 丢弃
                if (frame->pts < targetPTS)
                {
                    av_frame_unref(frame);
                    continue;
                }

                // 找到目标帧
                if(hasVideoStream)
                {
                    if(m_cudaAccelerate)
                    {
                        // Transfer data from GPU to CPU
                        if (frame->format == AV_PIX_FMT_CUDA)
                        {
                            if (av_hwframe_transfer_data(m_frameSw, frame, 0) < 0)
                            {
                                logger.error("Error transferring the data to system memory\n");
                                break;
                            }
                        }
                        else
                        {
                            logger.warning("Error format found, decoded frame with timestamp: %lld", frame->pts);
                        }
                        av_frame_ref(m_frame, m_frameSw);
                    }
                    else
                    {
                        av_frame_ref(m_frame, frame);
                    }

                    // 渲染
                    std::lock_guard<std::mutex> lock(m_renderMtx);
                    renderFrameRgb();
                    av_frame_unref(m_frame);
                }
                av_frame_unref(frame);
                throwing = false;
                break;
            }
        }
        av_packet_unref(packet);
    }
    av_frame_free(&frame);
    av_packet_free(&packet);

    m_systemClock->setTime(timeSecs);
    logger.info("seek complete");
}

float MediaManager::getCurrentProgress() const
{
    if(m_audioIndex >= 0)
        return m_audioLastPTS;
    else
        return m_videoLastPTS;
}

void MediaManager::changeVolume(int volume)
{
    m_sdlPlayer->setVolume(volume);
}

void MediaManager::changeSpeed(float speedFactor)
{
    logger.info("change speed to %0.2f", speedFactor);

    if(m_audioIndex >= 0)
    {
        soundtouch_setTempo(m_soundTouch, speedFactor);
        m_sdlPlayer->audioChangeSpeed(speedFactor);
    }

    m_systemClock->setSpeed(speedFactor);
    m_speedFactor = speedFactor;
}

void MediaManager::delayMs(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#define FREE_PTR(ptr, freeFunc) if(ptr) { freeFunc(&ptr); ptr = nullptr; }
void MediaManager::close()
{
    logger.debug("call close function");
    m_thread_quit = true;
    m_mediaQueue->signalExit();
    m_mediaQueue->clear();

    // 安全退出
    while(m_thread_media_read_exited == false ||
          m_thread_video_decode_exited == false ||
          m_thread_audio_decode_exited == false ||
          m_thread_video_display_exited == false ||
          m_thread_audio_display_exited == false)
    {
        logger.debug("waitting thread exit.");
        delayMs(20);
    }

    // 清理资源，注意顺序避免崩溃
    /*m_frameBuf和m_swsCtx不可轻易释放，避免渲染函数访问造成崩溃，比如Qt重绘仍会使用该内存*/

    if (m_sdlPlayer)
    {
        delete m_sdlPlayer;
        m_sdlPlayer = nullptr;
    }

    if(m_soundTouch)
    {
        soundtouch_destroyInstance(m_soundTouch);
        m_soundTouch = nullptr;
    }

    if (m_outBuf)
    {
        av_free(m_outBuf);
        m_outBuf = nullptr;
    }

    if(m_deviceCtx)
    {
        m_videoCodecCtx->hw_device_ctx = nullptr;
        av_buffer_unref(&m_deviceCtx);
        m_deviceCtx = nullptr;
    }

    FREE_PTR(m_swrCtx, swr_free)
    FREE_PTR(m_videoCodecCtx, avcodec_free_context)
    FREE_PTR(m_audioCodecCtx, avcodec_free_context)
    FREE_PTR(m_formatCtx, avformat_close_input)

    FREE_PTR(m_frame, av_frame_free)
    FREE_PTR(m_frameSw, av_frame_free)
    FREE_PTR(m_frameRgb, av_frame_free)

    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;
    m_videoIndex = -1;
    m_audioIndex = -1;
    m_systemClock->stop();

    // 安全退出标志
    m_thread_safe_exited = true;
    logger.info("all thread exit.");
}

void MediaManager::initVideoCodec()
{
    int ret = 0;
    // 创建音视频解码器上下文
    m_videoCodecCtx = avcodec_alloc_context3(nullptr);
    // 解码器上下文获取参数
    ret = avcodec_parameters_to_context(m_videoCodecCtx, m_formatCtx->streams[m_videoIndex]->codecpar);
    if(ret < 0)
        logger.error("Error occurred in avcodec_parameters_to_context");
    // 查找解码器
    m_videoCodec = avcodec_find_decoder(m_videoCodecCtx->codec_id);
    if(!m_videoCodec)
        logger.error("Error occurred in avcodec_find_decoder");

    if(m_cudaAccelerate)
    {
        enum AVHWDeviceType type = av_hwdevice_find_type_by_name("cuda");
        if (type == AV_HWDEVICE_TYPE_NONE)
        {
            logger.error("cuda device type not found\n");
            return;
        }

        if (av_hwdevice_ctx_create(&m_deviceCtx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) < 0) {
            logger.error("Failed to create CUDA hardware device context");
            return;
        }
        m_videoCodecCtx->hw_device_ctx = m_deviceCtx;

        m_videoCodecCtx->get_format = [](AVCodecContext *ctx, const AVPixelFormat *pix_fmts) {
            for (const AVPixelFormat *p = pix_fmts; *p != -1; p++) {
                if (*p == AV_PIX_FMT_CUDA) {
                    return *p;
                }
            }
            return pix_fmts[0];
        };
    }

    // 打开解码器并绑定上下文
    ret = avcodec_open2(m_videoCodecCtx, m_videoCodec, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avcodec_open2");
}

void MediaManager::initAudioCodec()
{
    int ret = 0;
    m_audioCodecCtx = avcodec_alloc_context3(nullptr);

    ret = avcodec_parameters_to_context(m_audioCodecCtx, m_formatCtx->streams[m_audioIndex]->codecpar);
    if(ret < 0)
        logger.error("Error occurred in avcodec_parameters_to_context");

    m_audioCodec = avcodec_find_decoder(m_audioCodecCtx->codec_id);
    if(!m_audioCodec)
        logger.error("Error occurred in avcodec_find_decoder");

    ret = avcodec_open2(m_audioCodecCtx, m_audioCodec, nullptr);
    if(ret < 0)
        logger.error("Error occurred in avcodec_open2");
}

void MediaManager::initAudioDevice()
{
    ///////音频设备初始化///////

    // 创建音频数据缓冲区
    m_outBuf = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * m_audioCodecCtx->ch_layout.nb_channels);

    // 创建重采样上下文
    m_swrCtx = swr_alloc();

    // 分配重采样的上下文信息
    swr_alloc_set_opts2(&m_swrCtx,
                           &m_audioCodecCtx->ch_layout,
                           AV_SAMPLE_FMT_FLT,
                           m_audioCodecCtx->sample_rate,
                           &m_audioCodecCtx->ch_layout,
                           m_audioCodecCtx->sample_fmt,
                           m_audioCodecCtx->sample_rate,
                           0,
                           nullptr);

    // swr上下文初始化
    swr_init(m_swrCtx);

    // 初始化 SoundTouch 实例
    m_soundTouch = soundtouch_createInstance();
    soundtouch_setSampleRate(m_soundTouch, m_audioCodecCtx->sample_rate);    // 设置采样率
    soundtouch_setChannels(m_soundTouch, m_audioCodecCtx->ch_layout.nb_channels);         // 设置通道数
    soundtouch_setTempo(m_soundTouch, m_speedFactor);                           // 设置倍速播放

    // 开启音频设备
    m_sdlPlayer = new SdlPlayer;
    m_sdlPlayer->initAudioDevice(m_audioCodecCtx, AV_SAMPLE_FMT_FLT);       //SDL仅支持部分音频格式
}

// 读取线程
int MediaManager::thread_media_read()
{
    AVPacket* packet = av_packet_alloc();

    while(m_thread_quit == false)
    {
        // 暂停时停止读取和解码线程，防止跳转时出错
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        // 队列满则阻塞
        if(m_mediaQueue->getVideoPacketCount() >= MAX_VIDEO_PACKETS || m_mediaQueue->getAudioPacketCount() >= MAX_AUDIO_PACKETS)
        {
            delayMs(10);
            continue;
        }

//        logger.debug("video packet: %d, audio packet: %d",m_mediaQueue->getVideoPacketCount(), m_mediaQueue->getAudioPacketCount());

        if(av_read_frame(m_formatCtx, packet) < 0)
            break;

        // 放入队列
        if(packet->stream_index == m_videoIndex)
            m_mediaQueue->pushVideoPacket(packet);
        else if(packet->stream_index == m_audioIndex)
            m_mediaQueue->pushAudioPacket(packet);

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    m_thread_media_read_exited = true;
    logger.debug("media read thread exit.");

    return 0;
}

int MediaManager::thread_video_decode()
{
    int ret;
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = nullptr;

    //解码
    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        if(m_thread_media_read_exited && m_mediaQueue->getVideoPacketCount() == 0)  //播放完毕
            break;

        packet = m_mediaQueue->popVideoPacket();
        if(!packet)
            continue;

        std::unique_lock<std::mutex> lock(m_decodeMtx);     //手动控制加锁和解锁，减小细粒度
        avcodec_send_packet(m_videoCodecCtx, packet);
        lock.unlock();

        while(m_thread_quit == false)
        {
            /*
             * 防止跳帧时该线程仍在该while中循环，
             * 否则avcodec_receive_frame与跳帧线程中的send/receive竞争解码器资源，
             * 造成程序奔溃。
            */
            if(m_thread_pause)
            {
                delayMs(10);
                continue;
            }

            if(m_mediaQueue->getVideoFrameCount() >= MAX_VIDEO_FRAMES)
            {
                delayMs(10);
                continue;
            }

            ret = avcodec_receive_frame(m_videoCodecCtx, frame);
            if(ret < 0)
            {
                if(ret == AVERROR_EOF)
                    logger.info("Media playback finished.");  // 视频解码结束
                break;
            }

            m_mediaQueue->pushVideoFrame(frame);

            av_frame_unref(frame);
        }
    }

    av_frame_free(&frame);
    m_thread_video_decode_exited = true;
    logger.debug("video decode thread exit.");

    return 0;
}

int MediaManager::thread_audio_decode()
{
    int ret;
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = nullptr;

    //解码
    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        if(m_thread_media_read_exited && m_mediaQueue->getAudioPacketCount() == 0)  //播放完毕
            break;

        packet = m_mediaQueue->popAudioPacket();
        if(!packet)
            continue;

        std::unique_lock<std::mutex> lock(m_decodeMtx);     //手动控制加锁和解锁，减小细粒度
        avcodec_send_packet(m_audioCodecCtx, packet);
        lock.unlock();

        while(m_thread_quit == false)
        {
            if(m_thread_pause)
            {
                delayMs(10);
                continue;
            }

            if(m_mediaQueue->getAudioFrameCount() >= MAX_AUDIO_FRAMES)
            {
                delayMs(10);
                continue;
            }

            ret = avcodec_receive_frame(m_audioCodecCtx, frame);

            if(ret < 0)
            {
                if(ret == AVERROR_EOF)
                    logger.info("Media playback finished.");  // 音频解码结束
                break;
            }

            m_mediaQueue->pushAudioFrame(frame);

            av_frame_unref(frame);
        }
    }

    av_frame_free(&frame);
    m_thread_audio_decode_exited = true;
    logger.debug("audio decode thread exit.");

    return 0;
}

//视频播放线程
int MediaManager::thread_video_display()
{
    AVFrame* frame = nullptr;

    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        if(m_thread_video_decode_exited && m_mediaQueue->getVideoFrameCount() == 0)  //播放完毕
            break;

        frame = m_mediaQueue->popVideoFrame();
        if(!frame)
            continue;

        if(m_cudaAccelerate)
        {
            // Transfer data from GPU to CPU
            if (frame->format == AV_PIX_FMT_CUDA)
            {
                if (av_hwframe_transfer_data(m_frameSw, frame, 0) < 0)
                {
                    logger.error("Error transferring the data to system memory\n");
                    break;
                }
            }
            else
            {
                logger.warning("Error format found, decoded frame with timestamp: %lld", frame->pts);
            }
            av_frame_ref(m_frame, m_frameSw);
        }
        else
        {
            av_frame_ref(m_frame, frame);
        }

        // 渲染
        if (m_renderMtx.try_lock()) // 尝试获取锁，非阻塞
        {
            std::lock_guard<std::mutex> lock(m_renderMtx, std::adopt_lock);  // std::adopt_lock:接管try_lock()锁定的互斥量
            renderFrameRgb();
        }
        else
        {
            logger.warning("can not get mtx, skip render");
        }

        // 延时控制
        renderDelayControl(frame);

        av_frame_unref(frame);
        av_frame_unref(m_frame);
    }
    av_frame_free(&frame);
    m_thread_video_display_exited = true;
    logger.debug("video display thread exit.");

    return 0;
}

//音频播放线程
int MediaManager::thread_audio_display()
{
    int ret;
    AVFrame* frame = nullptr;

    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }

        if(m_thread_audio_decode_exited && m_mediaQueue->getAudioFrameCount() == 0)  //播放完毕
            break;

        frame = m_mediaQueue->popAudioFrame();
        if(!frame)
            continue;

        ret = swr_convert(m_swrCtx, &m_outBuf, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
        if(ret < 0)
        {
            logger.error("Error while converting\n");
            break;
        }

        // 将重采样后的音频数据传递给 SoundTouch 进行处理
        soundtouch_putSamples(m_soundTouch, (const float*)m_outBuf, ret);

        // 获取处理后的样本
        int numSamplesProcessed = soundtouch_receiveSamples(m_soundTouch, (float*)m_outBuf, m_audioCodecCtx->frame_size / m_speedFactor);
//        logger.debug("putSamples = %d, numSamplesProcessed = %d", ret, numSamplesProcessed);

        // 音频PTS计算并记录
        m_audioLastPTS = frame->pts * av_q2d(m_formatCtx->streams[m_audioIndex]->time_base);

        // 等待SDL音频播放器完成当前的音频数据处理和输出
        while(m_sdlPlayer->m_audioLen > 0)
            delayMs(1);

        // 音频填充参数
        m_sdlPlayer->m_audioChunk = (unsigned char *)m_outBuf;
        m_sdlPlayer->m_audioPos = m_sdlPlayer->m_audioChunk;
        m_sdlPlayer->m_audioLen = numSamplesProcessed * m_audioCodecCtx->ch_layout.nb_channels * sizeof(float);  // 处理后的数据长度

        av_frame_unref(frame);
    }
    av_frame_free(&frame);
    m_thread_audio_display_exited = true;
    logger.debug("audio display thread exit.");

    return 0;
}

int MediaManager::thread_stream_convert()
{
    //1.定义输入输出格式上下文
    AVFormatContext* inputFormatCtx = nullptr;
    AVFormatContext* outputFormatCtx = nullptr;

    //2.打开输入文件
    int ret = avformat_open_input(&inputFormatCtx, m_inputStreamUrl.data(), nullptr, nullptr);
    if(ret < 0) return -1;

    //3.打开输出文件
    ret = avformat_alloc_output_context2(&outputFormatCtx, nullptr, "flv", m_outputStreamUrl.data());
    if(ret < 0) return -1;
    if(!outputFormatCtx) return -1;

    //4.分析流信息
    ret = avformat_find_stream_info(inputFormatCtx, nullptr);
    if(ret < 0) return -1;

    av_dump_format(inputFormatCtx, 0, m_inputStreamUrl.data(), 0);            //打印输入信息

    //5.获取流信息
    for(unsigned int i = 0; i < inputFormatCtx->nb_streams; i++)
    {
        AVStream *in_stream = inputFormatCtx->streams[i];
        AVStream *out_stream = avformat_new_stream(outputFormatCtx, nullptr);
        if(!out_stream) return -1;

        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if(ret < 0) return -1;

        out_stream->codecpar->codec_tag = 0;
    }

    //6.输出封装处理
    if(outputFormatCtx && !(outputFormatCtx->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&outputFormatCtx->pb, m_outputStreamUrl.data(), AVIO_FLAG_WRITE);
        if(ret < 0) return -1;
    }

    //7.写文件头
    ret = avformat_write_header(outputFormatCtx, nullptr);
    if(ret < 0) return -1;

    av_dump_format(outputFormatCtx, 0, m_outputStreamUrl.data(), 1);          //打印输出信息

    //查找是否有视频流
    AVMediaType printMediaType = AVMEDIA_TYPE_AUDIO;
    int videoIndex = av_find_best_stream(inputFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(videoIndex >= 0)
        printMediaType = AVMEDIA_TYPE_VIDEO;

    uint64_t frameIndex = 0;
    AVPacket *packet = av_packet_alloc();
    int64_t startTime = av_gettime();  //计时

    while(true)
    {
        //读取一个包
        if(av_read_frame(inputFormatCtx, packet) < 0)
            break;

        //转换包
        AVStream *inStream = inputFormatCtx->streams[packet->stream_index];
        AVStream *outStream = outputFormatCtx->streams[packet->stream_index];

        av_packet_rescale_ts(packet, inStream->time_base, outStream->time_base);     //输入、输出都是rtmp时，时基一样，可省略时间戳转换

        //输出到屏幕
        AVMediaType mediaType = inStream->codecpar->codec_type;
        if(mediaType == printMediaType)
        {
            if(frameIndex % 10 == 0)    //10帧1打印
            {
                uint64_t time = (av_gettime() - startTime) / AV_TIME_BASE;
                logger.debug("save frame: %llu , time: %llds", frameIndex, time);
            }
            frameIndex++;
        }

        //写包
        ret = av_interleaved_write_frame(outputFormatCtx, packet);
        if(ret < 0) return ret;

        //同步待开发
        delayMs(10);


        av_packet_unref(packet);
    }

    /* 写文件尾
    * 若是rtmp推流，可忽略以下报错信息，因为推流rtmp使用的flv格式不含时长和大小信息
    * [flv @ 0000000031af2880] Failed to update header with correct duration.
    * [flv @ 0000000031af2880] Failed to update header with correct filesize.
    */
    ret = av_write_trailer(outputFormatCtx);
    if(ret < 0) return ret;

    //关闭输入
    avformat_close_input(&inputFormatCtx);
    av_packet_free(&packet);

    if(outputFormatCtx && !(outputFormatCtx->flags & AVFMT_NOFILE))
    {
        avio_close(outputFormatCtx->pb);
    }
    avformat_free_context(outputFormatCtx);
    logger.info("push stream finished");

    return 0;
}

void MediaManager::renderDelayControl(AVFrame* frame)
{
    double currentVideoPTS = frame->pts * av_q2d(m_formatCtx->streams[m_videoIndex]->time_base);
    double delayDuration = 0.0;
    // 有音频时向音频流同步，无音频流则按PTS播放
    if(m_audioIndex >= 0)
        delayDuration = currentVideoPTS - m_audioLastPTS;
    else
        delayDuration = currentVideoPTS - m_systemClock->getTime();

//    logger.debug("time: %f", m_systemClock->getTime());
//    logger.debug("Current Video PTS: %f, Last PTS: %f, m_audioLastPTS: %f", currentVideoPTS, m_videoLastPTS, m_audioLastPTS);

    /* 注意：
     * 跳转后当视频流比音频流先渲染第一帧时，
     * m_audioLastPTS还处于跳转前的PTS，
     * 极大落后于新视频帧的PTS，因此可能出现长时间的延时，
     * 因此需要另外调整delayDuration的值
    */
    if (delayDuration > 0.0 && m_thread_quit == false && m_thread_pause == false)
    {
        if(delayDuration > 0.1 / m_speedFactor)
            delayDuration = 0.04 / m_speedFactor;

//        logger.debug("video delay: %f", delayDuration);
        av_usleep(delayDuration * AV_TIME_BASE);
    }

    // 记录当前视频PTS
    m_videoLastPTS = currentVideoPTS;
}

void MediaManager::frameYuvToRgb()
{
    AVPixelFormat srcFormat = m_cudaAccelerate ? AV_PIX_FMT_NV12 : m_videoCodecCtx->pix_fmt;

    uint8_t* tmpBuf = m_frameBuf;
    SwsContext* tmpSws = m_swsCtx;
    uint8_t* frameBuf = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_windowWidth, m_windowHeight, 1));
    SwsContext* swsCtx = sws_getContext(m_videoCodecCtx->width, m_videoCodecCtx->height, srcFormat,
                                   m_windowWidth, m_windowHeight, AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr);
    av_image_fill_arrays(m_frameRgb->data, m_frameRgb->linesize, frameBuf, AV_PIX_FMT_RGB32, m_windowWidth, m_windowHeight, 1);
    m_frameBuf = frameBuf;
    m_swsCtx = swsCtx;
    if(tmpBuf)
        av_freep(&tmpBuf);
    if(tmpSws)
        sws_freeContext(tmpSws);
}

void MediaManager::frameResize(int width, int height, bool uniformScale)
{
    AVPixelFormat srcFormat = m_cudaAccelerate ? AV_PIX_FMT_NV12 : m_videoCodecCtx->pix_fmt;

    m_windowWidth = width;
    m_windowHeight = height;

    // 等比例调整
    if(uniformScale)
    {
        if (width / static_cast<double>(height) > m_aspectRatio)
            m_windowWidth = static_cast<int>(height * m_aspectRatio);       // 根据高度计算宽度
        else
            m_windowHeight = static_cast<int>(width / m_aspectRatio);       // 根据宽度计算高度
    }

//    logger.debug("m_windowWidth: %d, m_windowHeight: %d", m_windowWidth, m_windowHeight);


    // 缓冲区数量
    static uint8_t* tmpBuf[TMP_BUFFER_NUMBER] = {nullptr};
    static SwsContext* tmpSws[TMP_BUFFER_NUMBER] = {nullptr};
    static int count = 0;

//    logger.debug("av_malloc: %d", count);
//    logger.debug("av_free: %d", (count + 1) % TMP_BUFFER_NUMBER);

    // 创建新的缓冲区，需要预留空间
    tmpBuf[count] = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_windowWidth * 4, m_windowHeight * 4, 1));

    if(tmpBuf[(count + 1) % TMP_BUFFER_NUMBER])
        av_freep(&tmpBuf[(count + 1) % TMP_BUFFER_NUMBER]);

    // 创建新的 SwsContext 以转换图像
    tmpSws[count] = sws_getContext(m_videoCodecCtx->width, m_videoCodecCtx->height, srcFormat,
                                   m_windowWidth, m_windowHeight, AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if(tmpSws[(count + 1) % TMP_BUFFER_NUMBER])
    {
        sws_freeContext(tmpSws[(count + 1) % TMP_BUFFER_NUMBER]);
        tmpSws[(count + 1) % TMP_BUFFER_NUMBER] = nullptr;
    }

    // 使用新缓冲区填充数据
    av_image_fill_arrays(m_frameRgb->data, m_frameRgb->linesize, tmpBuf[count], AV_PIX_FMT_RGB32, m_windowWidth, m_windowHeight, 1);

    // 确保新缓冲区准备好后切换
    m_frameBuf = tmpBuf[count];  // 切换到新的缓冲区
    m_swsCtx = tmpSws[count];   // 切换到新的 SwsContext

    count++;
    if(count == TMP_BUFFER_NUMBER)
        count = 0;

    // 渲染
    std::lock_guard<std::mutex> lock(m_renderMtx);
    renderFrameRgb();
}

void MediaManager::renderFrameRgb()
{
    if(!m_frame || !m_frame->data[0] || !m_swsCtx)
    {
        logger.warning("data or swsCtx is null, skip render");
        return;
    }

    sws_scale(m_swsCtx,
              (const unsigned char* const*)m_frame->data,
              m_frame->linesize, 0,
              m_videoCodecCtx->height,
              m_frameRgb->data,
              m_frameRgb->linesize);

    // 调用回调函数，通知 GUI 渲染
    if (m_renderCallback)
#ifdef ENABLE_PYBIND
        m_renderCallback(reinterpret_cast<int64_t>(m_frameRgb->data[0]), m_windowWidth, m_windowHeight);
#else
        m_renderCallback(m_frameRgb->data[0], m_windowWidth, m_windowHeight);
#endif
    else
        logger.error("Render callback not set");
}

