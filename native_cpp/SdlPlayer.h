#ifndef SDLPLAYER_H
#define SDLPLAYER_H

#include <iostream>
#include <map>

extern "C"
{
#include "SDL.h"
#include "libavcodec/avcodec.h"
}

//  audio sample rates map from FFMPEG to SDL (only non planar)
static std::map<int,int> AUDIO_FORMAT_MAP = {
   // AV_SAMPLE_FMT_NONE = -1,
    {AV_SAMPLE_FMT_U8,  AUDIO_U8    },
    {AV_SAMPLE_FMT_S16, AUDIO_S16SYS},
    {AV_SAMPLE_FMT_S32, AUDIO_S32SYS},
    {AV_SAMPLE_FMT_FLT, AUDIO_F32SYS}
};


// 定义 SdlPlayer 类
class SdlPlayer {
public:
    SdlPlayer();
    ~SdlPlayer();

    bool initVideoDevice(int width, int height, bool RgbMode);
    bool initAudioDevice(AVCodecContext* audioCodecCtx, AVSampleFormat fmt);

    static void fill_audio(void * udata, Uint8 * stream, int len);

    void renderFrame(const AVFrame* frame);
    void renderFrameRgb(const AVFrame* frameRgb);
    void resize(int width, int height, bool RgbMode);
    void setVolume(int volume);
    void audioChangeSpeed(float speedFactor);

    unsigned int m_audioLen = 0;             //音频数据块的长度
    unsigned char *m_audioChunk = nullptr;   //指向新获取的音频数据块的指针，目前多余
    unsigned char *m_audioPos = nullptr;     //指向当前正在处理的音频数据位置的指针

private:
    SDL_Window* m_window;      // SDL窗口
    SDL_Renderer* m_renderer;  // SDL渲染器
    SDL_Texture* m_texture;    // SDL纹理
    SDL_Rect m_rect;           // SDL矩形

    SDL_AudioSpec m_wantSpec;
    int m_raw_frame_size;      //记录原始每帧样本数
    int m_volume;              //音量
};


#endif // SDLPLAYER_H
