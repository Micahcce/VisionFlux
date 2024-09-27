#ifndef SDLPLAYER_H
#define SDLPLAYER_H

#include <iostream>
#include <map>
#include "FrameQueue.h"
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


class AudioParams
{
public:
    int out_sample_rate;                 //采样率
    int out_channels;                    //通道数
    int out_nb_samples;                  //单个通道的样本数
    enum AVSampleFormat out_sample_fmt;  //声音格式

    int out_buffer_size;        //输出缓冲区大小
    unsigned char *outBuff;     //音频数据缓冲区
};

// 定义 SdlPlayer 类
class SdlPlayer {
public:
    SdlPlayer();
    ~SdlPlayer();

    bool initVideoDevice(int width, int height, bool RGBMode);
    bool initAudioDevice(AudioParams* audioParams);

    static void fill_audio(void * udata, Uint8 * stream, int len);

    void renderFrame(const AVFrame* frame);
    void renderFrameRGB(const AVFrame* frameRGB);
    void resize(int width, int height, bool RGBMode);
    void close();

    unsigned int m_audioLen = 0;          //音频数据块的长度
    unsigned char *m_audioChunk = NULL;   //指向新获取的音频数据块的指针，目前多余
    unsigned char *m_audioPos = NULL;     //指向当前正在处理的音频数据位置的指针

private:
    SDL_Window* m_window;      // SDL窗口
    SDL_Renderer* m_renderer;  // SDL渲染器
    SDL_Texture* m_texture;    // SDL纹理
    SDL_Rect m_rect;           // SDL矩形
};


#endif // SDLPLAYER_H
