#include "SdlPlayer.h"
#include "MediaManager.h"

bool isSdlInitialized = false;

// 自定义事件类型，全局只注册一次
Uint32 CREATE_WINDOW_EVENT = SDL_RegisterEvents(123);

// 构造函数
SdlPlayer::SdlPlayer(): m_window(nullptr), m_renderer(nullptr), m_texture(nullptr),
    m_volume(100), m_raw_frame_size(0)
{
    if (!isSdlInitialized)
    {
        std::thread(&SdlPlayer::thread_window_event).detach();

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }
        isSdlInitialized = true;
    }
}

// 析构函数
SdlPlayer::~SdlPlayer()
{
    if (m_texture != nullptr) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    if (m_renderer != nullptr) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

//    SDL_CloseAudio();
//    SDL_Quit();
}

// 创建窗口和渲染器
bool SdlPlayer::initVideoDevice(int width, int height, bool RgbMode)
{
    std::string windowName = "SdlDisplay_" + std::to_string(reinterpret_cast<uintptr_t>(this));     //需要避免命名冲突
    m_window = SDL_CreateWindow(windowName.data(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if(RgbMode)
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    else
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if(!m_texture)
        std::cerr << "Error occurred in SDL! SDL_Error: " << SDL_GetError() << std::endl;
    m_rect = SDL_Rect{0, 0, width, height};

    return true;
}

bool SdlPlayer::initAudioDevice(AVCodecContext* audioCodecCtx, AVSampleFormat fmt)
{
    //配置音频播放结构体SDL_AudioSpec，和SwrContext的音频重采样参数保持一致
    m_wantSpec.freq = audioCodecCtx->sample_rate;        //48000/1024=46.875帧
    m_wantSpec.format = AUDIO_FORMAT_MAP[fmt];
    m_wantSpec.channels = audioCodecCtx->ch_layout.nb_channels;
    m_wantSpec.silence = 0;
    m_wantSpec.samples = audioCodecCtx->frame_size;
    m_wantSpec.callback = fill_audio;                     //回调函数
    m_wantSpec.userdata = this;

    //保存原始采样率用于变速
    m_raw_frame_size = audioCodecCtx->frame_size;


    //打开音频设备
    SDL_CloseAudio();   //先关闭
    if(SDL_OpenAudio(&m_wantSpec, nullptr) < 0)
    {
        std::cerr << "Error occurred in SDL_OpenAudio" << std::endl;
        SDL_Quit();
    }

    //开始播放
    SDL_PauseAudio(0);

    return true;
}

// 渲染帧数据
void SdlPlayer::renderFrame(const AVFrame* frame) {
    SDL_UpdateYUVTexture(m_texture, nullptr,
                         frame->data[0], frame->linesize[0],
                         frame->data[1], frame->linesize[1],
                         frame->data[2], frame->linesize[2]);
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void SdlPlayer::renderFrameRgb(const AVFrame *frameRgb)
{
    SDL_UpdateTexture(m_texture, nullptr,
                         frameRgb->data[0], frameRgb->linesize[0]);
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void SdlPlayer::resize(int width, int height, bool RgbMode)
{
    // 释放旧的资源
    if (m_renderer != nullptr) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    // 重新渲染器
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    }

    // 释放旧的资源
    if (m_texture != nullptr) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    // 重新创建纹理
    if(RgbMode)
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    else
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (m_texture == nullptr) {
        std::cerr << "Failed to create texture: " + std::string(SDL_GetError()) << std::endl;
    }

    // 修改显示区域
    m_rect = SDL_Rect{0, 0, width, height};
}


void SdlPlayer::setVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    m_volume = volume;
}

void SdlPlayer::audioChangeSpeed(float speedFactor)
{
    //需要修改每帧样本数
    SDL_CloseAudio();
    m_wantSpec.samples = m_raw_frame_size / speedFactor;

    //打开音频设备
    if(SDL_OpenAudio(&m_wantSpec, nullptr) < 0)
    {
        std::cerr << "Error occurred in SDL_OpenAudio" << std::endl;
        SDL_Quit();
    }

    //开始播放
    SDL_PauseAudio(0);
}

//回调函数
void SdlPlayer::fill_audio(void *udata, Uint8 *stream, int len)
{
    SdlPlayer* pThis = (SdlPlayer*) udata;

    SDL_memset(stream, 0, len);
    if(pThis->m_audioLen == 0)return;

    len = (len > pThis->m_audioLen ? pThis->m_audioLen : len);

    SDL_MixAudio(stream, pThis->m_audioPos, len, pThis->m_volume);

    pThis->m_audioPos += len;
    pThis->m_audioLen -= len;
}

void SdlPlayer::createWindow(MediaManager *mediaManager, AVCodecContext *videoCodecCtx)
{
    // 构造自定义事件
    SDL_Event event;
    SDL_zero(event);
    event.type = CREATE_WINDOW_EVENT;

    // 可选数据
    event.user.code = 0;
    event.user.data1 = mediaManager;
    event.user.data2 = videoCodecCtx;

    SDL_PushEvent(&event);  // 推送事件到主线程
}

void SdlPlayer::thread_window_event()
{
    std::map<Uint32, MediaManager*> sdlManager;

    SDL_Event event;                    //定义事件

    while(true)
    {
        SDL_WaitEvent(&event);

        if (event.type == SDL_WINDOWEVENT)
        {
            for(std::pair pair : sdlManager)
            {
                Uint32 windowsId = pair.first;
                if (windowsId == event.window.windowID)
                {
                    MediaManager* mediaManager = pair.second;
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        mediaManager->setThreadQuit(true);
                        mediaManager->close();
                        break;
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        // 重新初始化渲染资源
                        mediaManager->getSdlPlayer()->resize(event.window.data1, event.window.data2, true);
                        break;
                    }
                }
            }
        }
        else if(event.type == SDL_KEYDOWN)
        {
            if(event.key.keysym.sym == SDLK_SPACE)  //空格键暂停
            {
                for(std::pair pair : sdlManager)
                {
                    Uint32 windowsId = pair.first;
                    MediaManager* mediaManager = pair.second;
                    if (windowsId == event.window.windowID)
                        mediaManager->setThreadPause(!mediaManager->getThreadPause());
                }
            }
        }
        else if (event.type == CREATE_WINDOW_EVENT)
        {
            MediaManager* mediaManager = static_cast<MediaManager*>(event.user.data1);
            AVCodecContext* codecCtx = (AVCodecContext*)event.user.data2;
            //创建窗口（要和Event在同一线程）
            if (codecCtx != nullptr)
            {
                int width = codecCtx->width;
                int height = codecCtx->height;
                mediaManager->getSdlPlayer()->initVideoDevice(width, height, true);
            }
            else
                mediaManager->getSdlPlayer()->initVideoDevice(400, 300, true);

            sdlManager[mediaManager->getSdlPlayer()->getWindowId()] = mediaManager;
        }
    }
}
