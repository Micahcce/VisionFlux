#ifndef MEDIAMANAGER_H
#define MEDIAMANAGER_H

#include <iostream>
#include <stdexcept>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include "FrameQueue.h"
#include "SdlPlayer.h"
#include "Logger.h"

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
    //转码保存
    //拉流播放
    //拉流保存
    //推流

    //修改进度
    void seekFrameByVideoStream(int timeSecs);
    void seekFrameByAudioStream(int timeSecs);

    //获取当前进度
    float getCurrentProgress();

    //获取流索引
    int getAudioIndex() {return m_audioIndex;}
    int getVideoIndex() {return m_videoIndex;}

    //最大音频帧
    enum
    {
        MAX_AUDIO_FRAME_SIZE = 192000,       // 1 second of 48khz 32bit audio    //48000 * (32/8)
        MAX_NODE_NUMBER = 50                 // 帧缓冲数量过少（如1、2）当连续解码音频或视频帧时，会使解码线程阻塞导致无法继续解码
    };

    //线程状态
    bool getThreadSafeExited() {return m_thread_safe_exited;}
    void setThreadQuit(bool status) {m_thread_quit = status;}
    void setThreadPause(bool status) {m_thread_pause = status;}

    //关闭线程与回收资源
    void close();

    //设置渲染回调函数
    using RenderCallback = std::function<void(uint8_t*, int, int, float)>;
    void setRenderCallback(RenderCallback callback) {m_renderCallback = std::move(callback);}

    SdlPlayer* getSdlPlayer() {return m_sdlPlayer;}

private:
    void initVideoCodec();
    void initAudioCodec();
    void initAudioDevice();
    void videoDelayControl(AVFrame* frame);
    void frameYuvToRgb();
    void delayMs(int ms);

    //线程函数
    int thread_media_decode();
    int thread_video_display();
    int thread_audio_display();

    // 静态包装函数，因为SDL线程不支持使用成员函数
    static int decodeThreadEntry(void* ptr)
    {
        MediaManager* mediaManager = static_cast<MediaManager*>(ptr);
        return mediaManager->thread_media_decode();
    }
    static int videoThreadEntry(void* ptr)
    {
        MediaManager* mediaManager = static_cast<MediaManager*>(ptr);
        return mediaManager->thread_media_decode();
    }
    static int audioThreadEntry(void* ptr)
    {
        MediaManager* mediaManager = static_cast<MediaManager*>(ptr);
        return mediaManager->thread_media_decode();
    }


    RenderCallback m_renderCallback = nullptr;      //回调，用于GUI渲染

    FrameQueue* m_frameQueue;
    SdlPlayer* m_sdlPlayer;

    //媒体数据相关，其中Index同时用于音视频流是否存在的判断
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

    //线程状态变量
    std::atomic<bool> m_thread_quit;
    std::atomic<bool> m_thread_pause;
    bool m_thread_safe_exited;
    bool m_thread_decode_exited;
    bool m_thread_video_exited;
    bool m_thread_audio_exited;

    //最后一帧的PTS，已转换为秒数
    double m_videoLastPTS;
    double m_audioLastPTS;
};

#endif // MEDIAMANAGER_H
