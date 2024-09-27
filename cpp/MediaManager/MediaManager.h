#ifndef MEDIAMANAGER_H
#define MEDIAMANAGER_H

#include <iostream>
#include <stdexcept>
#include <map>
#include <chrono>
#include <thread>
#include <functional>
#include "FrameQueue.h"
#include "SdlPlayer.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
}


class MediaManager
{
public:
    MediaManager();
    ~MediaManager();

    //获取数据
    AVFormatContext* getMediaInfo(const char* filePath);
    //解码播放
    void decodeToPlay(const char* filePath);
    //解码保存
    //拉流播放
    //拉流保存
    //推流
    //转码保存

    //最大音频帧
    enum
    {
        MAX_AUDIO_FRAME_SIZE = 192000,       // 1 second of 48khz 32bit audio    //48000 * (32/8)
        MAX_NODE_NUMBER = 20
    };

    static int thread_media_decode(void* data);
    static int thread_video_display(void* data);
    static int thread_audio_display(void* data);

    bool getThreadQuit() {return m_thread_quit;}
    bool getThreadPause() {return m_thread_pause;}
    void setThreadQuit(bool status) {m_thread_quit = status;}
    void setThreadPause(bool status) {m_thread_pause = status;}

    SdlPlayer* getSdlPlayer() {return m_sdlPlayer;}

    using RenderCallback = std::function<void(AVFrame* frameRGB, float aspectRatio)>;
    void setRenderCallback(RenderCallback callback) {this->m_renderCallback = callback;}

    void videoDelayContrl();
    void frameYuvToRgb();

private:
    void delayMs(int ms);
    void close();

    RenderCallback m_renderCallback;      //回调，用于GUI渲染

    FrameQueue* m_frameQueue;
    SdlPlayer* m_sdlPlayer;

    //媒体数据相关
    AVFormatContext* m_pFormatCtx;
    int m_videoIndex = -1;
    int m_audioIndex = -1;
    AVCodecContext* m_pCodecCtx_video;
    AVCodecContext* m_pCodecCtx_audio;
    const AVCodec* m_pCodec_video;
    const AVCodec* m_pCodec_audio;

    //视频信息
    float m_aspectRatio;
    bool m_RGBMode;

    //音频变量
    AudioParams* m_pAudioParams;
    SwrContext *m_swrCtx;

    //视频转换变量
    AVFrame *m_frameRGB;
    SwsContext* m_pSwsCtx;

    //线程变量
    bool m_thread_quit = false;
    bool m_thread_pause = true;

    int64_t m_startTime;
};

#endif // MEDIAMANAGER_H
