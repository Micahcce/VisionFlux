#include "MediaManager.h"


MediaManager::MediaManager()
    : m_renderCallback(nullptr),
      m_frameQueue(nullptr),
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
      m_thread_decode_exited(true),
      m_thread_video_exited(true),
      m_thread_audio_exited(true),
      m_videoLastPTS(0.0),
      m_audioLastPTS(0.0),
      m_speedFactor(1.0)
{
//    av_log_set_level(AV_LOG_DEBUG);
//    logger.setLogLevel(LogLevel::INFO);
    logger.debug("avformat_version :%d", avformat_version());

    m_rgbMode = true;       // Ŀǰ����SDL��Ч��Qtֻ��ΪRGB��Ⱦ
    m_frameQueue = new FrameQueue;
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

    //1.����������
    m_formatCtx = avformat_alloc_context();

    //2.���ļ�
    ret = avformat_open_input(&m_formatCtx, filePath.data(), nullptr, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_open_input");
        avformat_close_input(&m_formatCtx);
        return false;
    }

    //3.�����Ļ�ȡ����Ϣ
    ret = avformat_find_stream_info(m_formatCtx, nullptr);
    if(ret < 0)
    {
        logger.error("Error occurred in avformat_find_stream_info");
        avformat_close_input(&m_formatCtx);
        return false;
    }

    //4.������Ƶ������Ƶ��
    m_videoIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(m_videoIndex < 0)
        logger.warning("Not found video stream");

    m_audioIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(m_audioIndex < 0)
        logger.warning("Not found audio stream");

    //5.��ȡ��Ƶ����
    av_dump_format(m_formatCtx, -1, nullptr, 0);


    // ��ر�����ʼ��
    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;
    m_thread_quit = false;
    m_thread_pause = false;
    m_thread_safe_exited = false;
    m_thread_decode_exited = false;
    m_thread_video_exited = false;
    m_thread_audio_exited = false;
    m_frameQueue->reset();

    if(m_videoIndex < 0)
        m_thread_video_exited = true;
    if(m_audioIndex < 0)
        m_thread_audio_exited = true;

    // ��Ƶ��
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
            //���ת�����֡����
            m_frameSw = av_frame_alloc();
            m_frameSw->format = AV_PIX_FMT_NV12;        //cuda���ٺ�Ĭ�����NV12��ʽ
            m_frameSw->width = m_videoCodecCtx->width;
            m_frameSw->height = m_videoCodecCtx->height;
            av_frame_get_buffer(m_frameSw, 0); // ���뻺����
        }

        if(m_rgbMode)
            frameResize(m_windowWidth, m_windowHeight, true);
//            frameYuvToRgb();

        std::thread videoThread(&MediaManager::thread_video_display, this);
        videoThread.detach();
    }

    // ��Ƶ��
    if(m_audioIndex >= 0)
    {
        initAudioCodec();
        initAudioDevice();

        std::thread audioThread(&MediaManager::thread_audio_display, this);
        audioThread.detach();
    }

    // �����߳�
    std::thread decodeThread(&MediaManager::thread_media_decode, this);
    decodeThread.detach();

    // ����ϵͳ���ã�Ŀǰֻ���ڵ���Ƶ������Ⱦ��ʱ����
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

void MediaManager::seekFrameByStream(int timeSecs, bool hasVideoStream)
{
    m_frameQueue->clear();

    // ���ȸ�����Ƶ֡��seek����Ϊ��Ƶ֡�Ľ��벻��ҪI֡����ת����ܻ�Ӱ�쵽��Ƶ֡�Ľ���
    int streamIndex = hasVideoStream ? m_videoIndex : m_audioIndex;
    AVCodecContext* codecCtx = hasVideoStream ? m_videoCodecCtx : m_audioCodecCtx;

    AVRational time_base = m_formatCtx->streams[streamIndex]->time_base;
    logger.debug("time_base.num: %d", time_base.num);
    logger.debug("time_base.den: %d", time_base.den);

    // ��Ŀ��ʱ��ת��ΪPTS
    int64_t targetPTS = av_rescale_q(timeSecs * AV_TIME_BASE, AV_TIME_BASE_Q, time_base); // ��ʱ��ת��ΪPTS
    logger.info("seek PTS: %d", targetPTS);

    // ʹ�� AVSEEK_FLAG_BACKWARD ��ȷ����ǰ��������� I ֡
    if (av_seek_frame(m_formatCtx, streamIndex, targetPTS, AVSEEK_FLAG_BACKWARD) < 0)
    {
        logger.error("Error seeking to position.");
        return;
    }

    if(m_videoIndex >= 0)
        avcodec_flush_buffers(m_videoCodecCtx);
    if(m_audioIndex >= 0)
        avcodec_flush_buffers(m_audioCodecCtx);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool throwing = true;

    // ��������֡
    while (throwing)
    {
        if (av_read_frame(m_formatCtx, packet) < 0)
            break; // û�и����֡

        if (packet->stream_index == streamIndex)
        {
            avcodec_send_packet(codecCtx, packet);
            while (avcodec_receive_frame(codecCtx, frame) >= 0)
            {
//                logger.debug("throw frame type: %d ,pts: %d", frame->pict_type, frame->pts);
                // ����
                if (frame->pts < targetPTS)
                {
                    av_frame_unref(frame);
                    continue;
                }

                // �ҵ�Ŀ��֡
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

                    // ��Ⱦ
                    std::lock_guard<std::mutex> lock(renderMtx);
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


// ��Ƶ�����߳�
int MediaManager::thread_media_decode()
{
    int ret;

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    //����
    while(m_thread_quit == false)
    {
        if(m_thread_pause)
        {
            delayMs(10);
            continue;
        }
//        logger.debug("video frame count: %d, audio frame count: %d", m_frameQueue->getVideoFrameCount(), m_frameQueue->getAudioFrameCount());

        if(av_read_frame(m_formatCtx, packet) < 0)
            break;

        if(packet->stream_index == m_videoIndex)
        {
            avcodec_send_packet(m_videoCodecCtx, packet);

            while(1)
            {
                /*
                 * ��ֹ��֡ʱ���߳����ڸ�while��ѭ����
                 * ����avcodec_receive_frame����֡�߳��е�send/receive������������Դ��
                 * ��ɳ�������
                */
                if(m_thread_pause && !m_thread_quit)
                {
                    delayMs(10);
                    continue;
                }

                ret = avcodec_receive_frame(m_videoCodecCtx, frame);

                if(ret < 0)
                {
                    if(ret == AVERROR_EOF)
                        logger.info("Media playback finished.");  // ��Ƶ�������
                    break;
                }

                while(m_frameQueue->getVideoFrameCount() >= MAX_VIDEO_FRAMES && !m_thread_quit)
                    delayMs(10);

                m_frameQueue->pushVideoFrame(frame);

                av_frame_unref(frame);
            }
        }
        else if(packet->stream_index == m_audioIndex)
        {
            avcodec_send_packet(m_audioCodecCtx, packet);

            while(1)
            {
                if(m_thread_pause && !m_thread_quit)
                {
                    delayMs(10);
                    continue;
                }

                ret = avcodec_receive_frame(m_audioCodecCtx, frame);

                if(ret < 0)
                {
                    if(ret == AVERROR_EOF)
                        logger.info("Media playback finished.");  // ��Ƶ�������
                    break;
                }

                while(m_frameQueue->getAudioFrameCount() >= MAX_AUDIO_FRAMES && !m_thread_quit)
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
    logger.debug("media decode thread exit.");

    return 0;
}

void MediaManager::delayMs(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void MediaManager::close()
{
    logger.debug("closing");
    m_thread_quit = true;
    m_frameQueue->signalExit();
    m_frameQueue->clear();

    // ��ȫ�˳�
    while(m_thread_decode_exited == false || m_thread_video_exited == false || m_thread_audio_exited == false)
    {
        logger.debug("waitting thread exit.");
        delayMs(20);
    }

    // ������Դ��ע��˳��������
    /*m_frameBuf��m_pSwsCtx���������ͷţ�������Ⱦ����������ɱ���������Qt�ػ��Ի�ʹ�ø��ڴ�*/

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

    if(m_swrCtx)
    {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }

    if(m_deviceCtx)
    {
        m_videoCodecCtx->hw_device_ctx = nullptr;
        av_buffer_unref(&m_deviceCtx);
        m_deviceCtx = nullptr;
    }

    if(m_videoCodecCtx)
    {
        avcodec_free_context(&m_videoCodecCtx);
        m_videoCodecCtx = nullptr;
    }

    if(m_audioCodecCtx)
    {
        avcodec_free_context(&m_audioCodecCtx);
        m_audioCodecCtx = nullptr;
    }

    if(m_formatCtx)
    {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }

    if(m_frame)
    {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if(m_frameSw)
    {
        av_frame_free(&m_frameSw);
        m_frameSw = nullptr;
    }

    if(m_frameRgb)
    {
        av_frame_free(&m_frameRgb);
        m_frameRgb = nullptr;
    }

    m_videoLastPTS = 0.0;
    m_audioLastPTS = 0.0;
    m_videoIndex = -1;
    m_audioIndex = -1;
    m_systemClock->stop();

    // ��ȫ�˳���־
    m_thread_safe_exited = true;
    logger.info("all thread exit.");
}

void MediaManager::initVideoCodec()
{
    int ret = 0;
    // ��������Ƶ������������
    m_videoCodecCtx = avcodec_alloc_context3(nullptr);
    // �����������Ļ�ȡ����
    ret = avcodec_parameters_to_context(m_videoCodecCtx, m_formatCtx->streams[m_videoIndex]->codecpar);
    if(ret < 0)
        logger.error("Error occurred in avcodec_parameters_to_context");
    // ���ҽ�����
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

    // �򿪽���������������
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
    ///////��Ƶ�豸��ʼ��///////

    // ������Ƶ���ݻ�����
    m_outBuf = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * m_audioCodecCtx->ch_layout.nb_channels);

    // �����ز���������
    m_swrCtx = swr_alloc();

    // �����ز�������������Ϣ
    swr_alloc_set_opts2(&m_swrCtx,
                           &m_audioCodecCtx->ch_layout, 
                           AV_SAMPLE_FMT_FLT,           
                           m_audioCodecCtx->sample_rate,
                           &m_audioCodecCtx->ch_layout,      
                           m_audioCodecCtx->sample_fmt,      
                           m_audioCodecCtx->sample_rate,     
                           0,
                           nullptr);

    // swr�����ĳ�ʼ��
    swr_init(m_swrCtx);

    // ��ʼ�� SoundTouch ʵ��
    m_soundTouch = soundtouch_createInstance();
    soundtouch_setSampleRate(m_soundTouch, m_audioCodecCtx->sample_rate);    // ���ò�����
    soundtouch_setChannels(m_soundTouch, m_audioCodecCtx->ch_layout.nb_channels);         // ����ͨ����
    soundtouch_setTempo(m_soundTouch, m_speedFactor);                           // ���ñ��ٲ���

    // ������Ƶ�豸
    m_sdlPlayer = new SdlPlayer;
    m_sdlPlayer->initAudioDevice(m_audioCodecCtx, AV_SAMPLE_FMT_FLT);       //SDL��֧�ֲ�����Ƶ��ʽ
}

//��Ƶ�����߳�
int MediaManager::thread_video_display()
{
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

        // ��Ⱦ
        if (renderMtx.try_lock()) // ���Ի�ȡ����������
        {
            std::lock_guard<std::mutex> lock(renderMtx, std::adopt_lock);  // std::adopt_lock:�ӹ�try_lock()�����Ļ�����
            renderFrameRgb();
        }
        else
        {
            logger.warning("can not get mtx, skip render");
        }

        // ��ʱ����
        renderDelayControl(frame);

        av_frame_unref(frame);
        av_frame_unref(m_frame);
    }
    av_frame_free(&frame);
    m_thread_video_exited = true;
    logger.debug("video display thread exit.");

    return 0;
}


//��Ƶ�����߳�
int MediaManager::thread_audio_display()
{
    int ret;

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

        ret = swr_convert(m_swrCtx, &m_outBuf, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
        if(ret < 0)
        {
            logger.error("Error while converting\n");
            break;
        }

        // ���ز��������Ƶ���ݴ��ݸ� SoundTouch ���д���
        soundtouch_putSamples(m_soundTouch, (const float*)m_outBuf, ret);

        // ��ȡ����������
        int numSamplesProcessed = soundtouch_receiveSamples(m_soundTouch, (float*)m_outBuf, m_audioCodecCtx->frame_size / m_speedFactor);
//        logger.debug("putSamples = %d, numSamplesProcessed = %d", ret, numSamplesProcessed);

        // ��ƵPTS���㲢��¼
        m_audioLastPTS = frame->pts * av_q2d(m_formatCtx->streams[m_audioIndex]->time_base);

        // �ȴ�SDL��Ƶ��������ɵ�ǰ����Ƶ���ݴ�������
        while(m_sdlPlayer->m_audioLen > 0)
            delayMs(1);

        // ��Ƶ������
        m_sdlPlayer->m_audioChunk = (unsigned char *)m_outBuf;
        m_sdlPlayer->m_audioPos = m_sdlPlayer->m_audioChunk;
        m_sdlPlayer->m_audioLen = numSamplesProcessed * m_audioCodecCtx->ch_layout.nb_channels * sizeof(float);  // ���������ݳ���

        av_frame_unref(frame);
    }
    av_frame_free(&frame);
    m_thread_audio_exited = true;
    logger.debug("audio display thread exit.");

    return 0;
}

int MediaManager::thread_stream_convert()
{
    //1.�������������ʽ������
    AVFormatContext* inputFormatCtx = nullptr;
    AVFormatContext* outputFormatCtx = nullptr;

    //2.�������ļ�
    int ret = avformat_open_input(&inputFormatCtx, m_inputStreamUrl.data(), nullptr, nullptr);
    if(ret < 0) return -1;

    //3.������ļ�
    ret = avformat_alloc_output_context2(&outputFormatCtx, nullptr, "flv", m_outputStreamUrl.data());
    if(ret < 0) return -1;
    if(!outputFormatCtx) return -1;

    //4.��������Ϣ
    ret = avformat_find_stream_info(inputFormatCtx, nullptr);
    if(ret < 0) return -1;

    av_dump_format(inputFormatCtx, 0, m_inputStreamUrl.data(), 0);            //��ӡ������Ϣ

    //5.��ȡ����Ϣ
    for(unsigned int i = 0; i < inputFormatCtx->nb_streams; i++)
    {
        AVStream *in_stream = inputFormatCtx->streams[i];
        AVStream *out_stream = avformat_new_stream(outputFormatCtx, nullptr);
        if(!out_stream) return -1;

        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if(ret < 0) return -1;

        out_stream->codecpar->codec_tag = 0;
    }

    //6.�����װ����
    if(outputFormatCtx && !(outputFormatCtx->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&outputFormatCtx->pb, m_outputStreamUrl.data(), AVIO_FLAG_WRITE);
        if(ret < 0) return -1;
    }

    //7.д�ļ�ͷ
    ret = avformat_write_header(outputFormatCtx, nullptr);
    if(ret < 0) return -1;

    av_dump_format(outputFormatCtx, 0, m_outputStreamUrl.data(), 1);          //��ӡ�����Ϣ

    //�����Ƿ�����Ƶ��
    AVMediaType printMediaType = AVMEDIA_TYPE_AUDIO;
    int videoIndex = av_find_best_stream(inputFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(videoIndex >= 0)
        printMediaType = AVMEDIA_TYPE_VIDEO;

    uint64_t frameIndex = 0;
    AVPacket *packet = av_packet_alloc();
    int64_t startTime = av_gettime();  //��ʱ

    while(true)
    {
        //��ȡһ����
        if(av_read_frame(inputFormatCtx, packet) < 0)
            break;

        //ת����
        AVStream *inStream = inputFormatCtx->streams[packet->stream_index];
        AVStream *outStream = outputFormatCtx->streams[packet->stream_index];

        av_packet_rescale_ts(packet, inStream->time_base, outStream->time_base);     //���롢�������rtmpʱ��ʱ��һ������ʡ��ʱ���ת��

        //�������Ļ
        AVMediaType mediaType = inStream->codecpar->codec_type;
        if(mediaType == printMediaType)
        {
            if(frameIndex % 10 == 0)    //10֡1��ӡ
            {
                uint64_t time = (av_gettime() - startTime) / AV_TIME_BASE;
                logger.debug("save frame: %llu , time: %llds", frameIndex, time);
            }
            frameIndex++;
        }

        //д��
        ret = av_interleaved_write_frame(outputFormatCtx, packet);
        if(ret < 0) return ret;

        //ͬ��������
        delayMs(10);


        av_packet_unref(packet);
    }

    /* д�ļ�β
    * ����rtmp�������ɺ������±�����Ϣ����Ϊ����rtmpʹ�õ�flv��ʽ����ʱ���ʹ�С��Ϣ
    * [flv @ 0000000031af2880] Failed to update header with correct duration.
    * [flv @ 0000000031af2880] Failed to update header with correct filesize.
    */
    ret = av_write_trailer(outputFormatCtx);
    if(ret < 0) return ret;

    //�ر�����
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
    // ����Ƶʱ����Ƶ��ͬ��������Ƶ����PTS����
    if(m_audioIndex >= 0)
        delayDuration = currentVideoPTS - m_audioLastPTS;
    else
        delayDuration = currentVideoPTS - m_systemClock->getTime();

//    logger.debug("time: %f", m_systemClock->getTime());
//    logger.debug("Current Video PTS: %f, Last PTS: %f, m_audioLastPTS: %f", currentVideoPTS, m_videoLastPTS, m_audioLastPTS);

    /* ע�⣺
     * ��ת����Ƶ������Ƶ������Ⱦ��һ֡ʱ��
     * m_audioLastPTS��������תǰ��PTS��
     * �������������Ƶ֡��PTS����˿��ܳ��ֳ�ʱ�����ʱ��
     * �����Ҫ�������delayDuration��ֵ
    */
    if (delayDuration > 0.0 && m_thread_quit == false && m_thread_pause == false)
    {
        if(delayDuration > 0.1 / m_speedFactor)
            delayDuration = 0.04 / m_speedFactor;

//        logger.debug("video delay: %f", delayDuration);
        av_usleep(delayDuration * AV_TIME_BASE);
    }

    // ��¼��ǰ��ƵPTS
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

    // �ȱ�������
    if(uniformScale)
    {
        if (width / static_cast<double>(height) > m_aspectRatio)
            m_windowWidth = static_cast<int>(height * m_aspectRatio);       // ���ݸ߶ȼ�����
        else
            m_windowHeight = static_cast<int>(width / m_aspectRatio);       // ���ݿ�ȼ���߶�
    }

//    logger.debug("m_windowWidth: %d, m_windowHeight: %d", m_windowWidth, m_windowHeight);


    // ����������
    static uint8_t* tmpBuf[TMP_BUFFER_NUMBER] = {nullptr};
    static SwsContext* tmpSws[TMP_BUFFER_NUMBER] = {nullptr};
    static int count = 0;

//    logger.debug("av_malloc: %d", count);
//    logger.debug("av_free: %d", (count + 1) % TMP_BUFFER_NUMBER);

    // �����µĻ���������ҪԤ���ռ�
    tmpBuf[count] = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_windowWidth * 4, m_windowHeight * 4, 1));

    if(tmpBuf[(count + 1) % TMP_BUFFER_NUMBER])
        av_freep(&tmpBuf[(count + 1) % TMP_BUFFER_NUMBER]);

    // �����µ� SwsContext ��ת��ͼ��
    tmpSws[count] = sws_getContext(m_videoCodecCtx->width, m_videoCodecCtx->height, srcFormat,
                                   m_windowWidth, m_windowHeight, AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if(tmpSws[(count + 1) % TMP_BUFFER_NUMBER])
    {
        sws_freeContext(tmpSws[(count + 1) % TMP_BUFFER_NUMBER]);
        tmpSws[(count + 1) % TMP_BUFFER_NUMBER] = nullptr;
    }

    // ʹ���»������������
    av_image_fill_arrays(m_frameRgb->data, m_frameRgb->linesize, tmpBuf[count], AV_PIX_FMT_RGB32, m_windowWidth, m_windowHeight, 1);

    // ȷ���»�����׼���ú��л�
    m_frameBuf = tmpBuf[count];  // �л����µĻ�����
    m_swsCtx = tmpSws[count];   // �л����µ� SwsContext

    count++;
    if(count == TMP_BUFFER_NUMBER)
        count = 0;

    // ��Ⱦ
    std::lock_guard<std::mutex> lock(renderMtx);
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

    // ���ûص�������֪ͨ GUI ��Ⱦ
    if (m_renderCallback)
#ifdef ENABLE_PYBIND
        m_renderCallback(reinterpret_cast<int64_t>(m_frameRgb->data[0]), m_windowWidth, m_windowHeight);
#else
        m_renderCallback(m_frameRgb->data[0], m_windowWidth, m_windowHeight);
#endif
    else
        logger.error("Render callback not set");
}

