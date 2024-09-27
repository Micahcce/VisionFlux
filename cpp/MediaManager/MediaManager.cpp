#include "MediaManager.h"
#define USE_SDL 0

MediaManager::MediaManager()
{
#if !USE_SDL

#endif
    m_RGBMode = true;       // 目前仅对SDL有效，Qt只能为RGB渲染
    std::cout << "avformat_version : " << avformat_version() << std::endl;
}

MediaManager::~MediaManager()
{
    close();
}


AVFormatContext* MediaManager::getMediaInfo(const char* filePath)
{
    //1.创建上下文
    AVFormatContext* formatCtx = avformat_alloc_context();

    //2.打开文件
    int ret = avformat_open_input(&formatCtx, filePath, nullptr, nullptr);
    if(ret < 0)
        throw std::runtime_error("Error occurred in avformat_open_input");

    //3.上下文获取流信息
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if(ret < 0)
        throw std::runtime_error("Error occurred in avformat_find_stream_info");

    //4.获取视频数据
    av_dump_format(formatCtx, -1, nullptr, 0);

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
        throw std::runtime_error("Error occurred in avformat_open_input");

    //3.上下文获取流信息
    ret = avformat_find_stream_info(m_pFormatCtx, nullptr);
    if(ret < 0)
        throw std::runtime_error("Error occurred in avformat_find_stream_info");

    //4.查找视频流和音频流
    m_videoIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if(m_videoIndex < 0)
        throw std::runtime_error("Error occurred in av_find_best_stream");

    m_audioIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if(m_audioIndex < 0)
        throw std::runtime_error("Error occurred in av_find_best_stream");

    //5.获取视频数据
    av_dump_format(m_pFormatCtx, -1, nullptr, 0);

    //6.创建音视频解码器上下文
    m_pCodecCtx_video = avcodec_alloc_context3(nullptr);
    m_pCodecCtx_audio = avcodec_alloc_context3(nullptr);

    //7.解码器上下文获取参数
    ret = avcodec_parameters_to_context(m_pCodecCtx_video, m_pFormatCtx->streams[m_videoIndex]->codecpar);
    if(ret < 0)
        throw std::runtime_error("Error occurred in avcodec_parameters_to_context");

    ret = avcodec_parameters_to_context(m_pCodecCtx_audio, m_pFormatCtx->streams[m_audioIndex]->codecpar);
    if(ret < 0)
        throw std::runtime_error("Error occurred in avcodec_parameters_to_context");

    //8.查找解码器
    m_pCodec_video = avcodec_find_decoder(m_pCodecCtx_video->codec_id);
    m_pCodec_audio = avcodec_find_decoder(m_pCodecCtx_audio->codec_id);
    if(!(m_pCodec_video || m_pCodecCtx_audio))
        throw std::runtime_error("Error occurred in avcodec_find_decoder");

    //9.打开解码器并绑定上下文
    ret = avcodec_open2(m_pCodecCtx_video, m_pCodec_video, nullptr);
    if(ret < 0)
        throw std::runtime_error("Error occurred in avcodec_open2");

    ret = avcodec_open2(m_pCodecCtx_audio, m_pCodec_audio, nullptr);
    if(ret < 0)
        throw std::runtime_error("Error occurred in avcodec_open2");

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

#if USE_SDL
    if(m_RGBMode)
        frameYuvToRgb();
    m_sdlPlayer->initVideoDevice(m_pCodecCtx_video->width, m_pCodecCtx_video->height, m_RGBMode);

    SDL_CreateThread(thread_media_decode, NULL, this);
    SDL_CreateThread(thread_video_display, NULL, this);
    SDL_CreateThread(thread_audio_display, NULL, this);

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
                this->setThreadPause(!this->getThreadPause());
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
    m_thread_pause = false;
    frameYuvToRgb();
    SDL_CreateThread(thread_media_decode, NULL, this);
    SDL_CreateThread(thread_video_display, NULL, this);
    SDL_CreateThread(thread_audio_display, NULL, this);

    //std::thread无法配合SDL_Event使用
//    std::thread m_decodeThread(thread_media_decode, this);
//    std::thread m_videoThread(thread_video_display, this);
//    std::thread m_audioThread(thread_audio_display, this);
//    m_decodeThread.join();
//    m_videoThread.join();
//    m_audioThread.join();
#endif
}


//视频解码线程
int MediaManager::thread_media_decode(void *data)
{
    MediaManager* pThis = (MediaManager*) data;
    int ret;

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    int64_t start_time = av_gettime() - pThis->m_startTime;

    //解码
    while(!pThis->m_thread_quit)
    {
        if(pThis->m_thread_pause)
        {
            pThis->delayMs(10);
            start_time += 10*1000;
            continue;
        }

        //读取一个包数据
        if(av_read_frame(pThis->m_pFormatCtx, packet) < 0)
            break;

        if(packet->stream_index == pThis->m_videoIndex)
        {
            //发送一个包数据
            avcodec_send_packet(pThis->m_pCodecCtx_video, packet);

            //显示数据
            while(1)
            {
                ret = avcodec_receive_frame(pThis->m_pCodecCtx_video, frame);

                if(ret < 0) break;
                else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                //队列节点太多则暂停解码
                while(pThis->m_frameQueue->getVideoFrameCount() >= MAX_NODE_NUMBER)
                    pThis->delayMs(10);

                //存入队列
                pThis->m_frameQueue->pushVideoFrame(frame);

                av_frame_unref(frame);
            }
        }
        else if(packet->stream_index == pThis->m_audioIndex)
        {
            avcodec_send_packet(pThis->m_pCodecCtx_audio, packet);      // 送一帧到解码器

            while(1)
            {
                ret = avcodec_receive_frame(pThis->m_pCodecCtx_audio, frame);
                if(ret < 0) break;
                else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                //队列节点太多则暂停解码
                while(pThis->m_frameQueue->getAudioFrameCount() >= MAX_NODE_NUMBER)
                    pThis->delayMs(10);

                //存入队列
                pThis->m_frameQueue->pushAudioFrame(frame);

                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    return 0;
}

void MediaManager::delayMs(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void MediaManager::close()
{
    swr_free(&m_swrCtx);
    avcodec_free_context(&m_pCodecCtx_video);
    avcodec_free_context(&m_pCodecCtx_audio);
    avformat_close_input(&m_pFormatCtx);
}

//视频播放线程
int MediaManager::thread_video_display(void* data)
{
    MediaManager* pThis = (MediaManager*) data;

    //渲染
    AVFrame* frame = av_frame_alloc();
    int64_t start_time = av_gettime() - pThis->m_startTime;              //获取从公元1970年1月1日0时0分0秒开始的微秒值

    while(!pThis->m_thread_quit)
    {
        if(pThis->m_thread_pause)
        {
            pThis->delayMs(10);
            start_time += 10*1000;
            continue;
        }

        frame = pThis->m_frameQueue->popVideoFrame();

        //渲染
#if USE_SDL
    if(pThis->m_RGBMode)
    {
        sws_scale(pThis->m_pSwsCtx,
                  (const unsigned char* const*)frame->data,
                  frame->linesize, 0,
                  pThis->m_pCodecCtx_video->height,
                  pThis->m_frameRGB->data,
                  pThis->m_frameRGB->linesize);
        pThis->m_sdlPlayer->renderFrameRGB(pThis->m_frameRGB);
    }
    else
        pThis->m_sdlPlayer->renderFrame(frame);
#else
        sws_scale(pThis->m_pSwsCtx,
                  (const unsigned char* const*)frame->data,
                  frame->linesize, 0,
                  pThis->m_pCodecCtx_video->height,
                  pThis->m_frameRGB->data,
                  pThis->m_frameRGB->linesize);

        // 调用回调函数，通知 GUI 渲染
        if (pThis->m_renderCallback)
        {
            pThis->m_renderCallback(pThis->m_frameRGB, pThis->m_aspectRatio);
        }
#endif

        //延时控制
        pThis->videoDelayContrl();

        av_frame_unref(frame);
    }
    av_frame_free(&frame);

    return 0;
}


//音频播放线程
int MediaManager::thread_audio_display(void *data)
{
    MediaManager* pThis = (MediaManager*) data;
    int ret;

    //渲染
    AVFrame* frame = av_frame_alloc();

    while(!pThis->m_thread_quit)
    {
        if(pThis->m_thread_pause)
        {
            pThis->delayMs(10);
            continue;
        }

        frame = pThis->m_frameQueue->popAudioFrame();

        ret = swr_convert(pThis->m_swrCtx, &pThis->m_pAudioParams->outBuff, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
        if(ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Error while converting\n");
            break;
        }

        while(pThis->m_sdlPlayer->m_audioLen > 0)
            pThis->delayMs(1);

        pThis->m_sdlPlayer->m_audioChunk = (unsigned char *)pThis->m_pAudioParams->outBuff;
        pThis->m_sdlPlayer->m_audioPos = pThis->m_sdlPlayer->m_audioChunk;
        pThis->m_sdlPlayer->m_audioLen = pThis->m_pAudioParams->out_buffer_size;

        av_frame_unref(frame);
    }
    av_frame_free(&frame);

    return 0;
}

void MediaManager::videoDelayContrl()
{
    //原方案（Qt不可用）
//        AVRational time_base_q = {1,AV_TIME_BASE};          // 1/1000000秒，即1us
//        int64_t pts_time = av_rescale_q(frame->pkt_dts, pThis->m_pFormatCtx->streams[pThis->m_videoIndex]->time_base, time_base_q);  //该函数将<参数二>时基中的<参数一>时间戳转换<参数三>时基下的<返回值>时间戳
//        int64_t now_time = av_gettime() - start_time;       // 计算当前程序已开始时间
//        std::clog << pts_time - now_time << std::endl;
//        if(pts_time > now_time)                             // 若应帧解码时间>当前时间对比，延时到应解码的时间再开始解下一帧(若无此延时则计算机将会很快解码显示完)
//              av_usleep(pts_time - now_time);

    //新方案（Qt不可用）（未测试）
//        static double lastPTS = 0.0;
//        double currentPTS = frame->pts * av_q2d(pThis->m_pFormatCtx->streams[pThis->m_videoIndex]->time_base);
//        if (lastPTS != 0.0) {
//            double delayDuration = currentPTS - lastPTS;
//            if (delayDuration > 0.0)
//            {
//                av_usleep(delayDuration);
//            }
//        }
//        lastPTS = currentPTS;

    //固定25帧
    delayMs(40);
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





