#include "SdlPlayer.h"

// 构造函数
SdlPlayer::SdlPlayer(): m_window(nullptr), m_renderer(nullptr), m_texture(nullptr),m_volume(100)
{
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

    SDL_CloseAudio();
    SDL_Quit();
}

// 创建窗口和渲染器
bool SdlPlayer::initVideoDevice(int width, int height, bool RGBMode)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_window = SDL_CreateWindow("SdlDisplay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if(RGBMode)
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    else
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if(!m_texture)
        std::cerr << "Error occurred in SDL! SDL_Error: " << SDL_GetError() << std::endl;
    m_rect = SDL_Rect{0, 0, width, height};

    return true;
}

bool SdlPlayer::initAudioDevice(AudioParams* audioParams)
{
    ///   SDL
    //配置音频播放结构体SDL_AudioSpec，和SwrContext的音频重采样参数保持一致
    m_wantSpec.freq = audioParams->out_sample_rate;        //48000/1024=46.875帧
    m_wantSpec.format = AUDIO_FORMAT_MAP[audioParams->out_sample_fmt];
    m_wantSpec.channels = audioParams->out_channels;
    m_wantSpec.silence = 0;
    m_wantSpec.samples = audioParams->out_nb_samples;
    m_wantSpec.callback = fill_audio;                     //回调函数
    m_wantSpec.userdata = this;

    //打开音频设备
    if(SDL_OpenAudio(&m_wantSpec, NULL) < 0)
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

void SdlPlayer::renderFrameRGB(const AVFrame *frameRGB)
{
    SDL_UpdateTexture(m_texture, nullptr,
                         frameRGB->data[0], frameRGB->linesize[0]);
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void SdlPlayer::resize(int width, int height, bool RGBMode)
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
    if(RGBMode)
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
    //通过修改采样率实现变速
    SDL_CloseAudio();
    m_wantSpec.freq = m_wantSpec.freq * speedFactor;

    //打开音频设备
    if(SDL_OpenAudio(&m_wantSpec, NULL) < 0)
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
