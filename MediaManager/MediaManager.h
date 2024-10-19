#ifndef MEDIAMANAGER_H
#define MEDIAMANAGER_H

#include <iostream>
#include <stdexcept>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include "FrameQueue.h"
#include "SdlPlayer.h"
#include "Logger.h"
#include "SystemClock.h"
#include "BmpAndWavAchieve.h"
#include <SoundTouchDLL.h>

#ifdef ENABLE_PYBIND
#include <pybind11/functional.h>
#else
#include <functional>
#endif

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
    AVFormatContext* getMediaInfo(const std::string& filePath);

    //解码播放
    bool decodeToPlay(const std::string& filePath);

    //流媒体转换
    /*
    * 网络流->本地流:拉流保存
    * 网络流->网络流:网络流转发
    * 本地流->本地流:格式转换
    * 本地流->网络流:推流
    */
    bool streamConvert(const std::string& inputStreamUrl, const std::string& outputStreamUrl);

    //修改进度
    void seekFrameByVideoStream(int timeSecs);
    void seekFrameByAudioStream(int timeSecs);

    //获取当前进度
    float getCurrentProgress() const;

    //获取流索引
    int getAudioIndex() const {return m_audioIndex;}
    int getVideoIndex() const {return m_videoIndex;}

    //变速
    void changeSpeed(float speedFactor);

    //尺寸调整
    void frameResize(int width, int height, bool uniformScale);

    //保存视频帧
    bool saveFrameToBmp(const std::string filePath, const std::string outputPath, int sec);

    //线程状态
    bool getThreadSafeExited() {return m_thread_safe_exited;}
    void setThreadQuit(bool status) {m_thread_quit = status;}
    void setThreadPause(bool status)
    {
        m_thread_pause = status;
        if(m_thread_pause)
            m_systemClock->pause();
        else
            m_systemClock->resume();
    }

    //关闭线程与回收资源
    void close();

    //设置渲染回调函数
#ifdef ENABLE_PYBIND
    using RenderCallback = std::function<void(int64_t, int, int)>;
#else
    using RenderCallback = std::function<void(uint8_t*, int, int)>;
#endif

    void setRenderCallback(RenderCallback callback) {m_renderCallback = std::move(callback);}

    SdlPlayer* getSdlPlayer() {return m_sdlPlayer;}

private:
    enum FrameQueueCapacity
    {
        //帧缓冲数量过少（如1、2）当连续解码音频或视频帧时，会使解码线程阻塞导致无法继续解码。
        //音频帧率通常为40多（采样率/每帧样本数）
        MAX_AUDIO_FRAMES = 30,
        MAX_VIDEO_FRAMES = 20
    };

    enum
    {
        MAX_AUDIO_FRAME_SIZE = 192000,       // 1 second of 48khz 32bit audio    //48000 * (32/8)
        TMP_BUFFER_NUMBER = 15               // 需要足够大小
    };

    void initVideoCodec();
    void initAudioCodec();
    void initAudioDevice();
    void renderDelayControl(AVFrame* frame);
    void frameYuvToRgb();
    void delayMs(int ms);

    //线程函数
    int thread_media_decode();
    int thread_video_display();
    int thread_audio_display();
    int thread_stream_convert();

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
    SystemClock* m_systemClock;
    SdlPlayer* m_sdlPlayer;
    HANDLE m_soundTouch;

    //媒体数据相关，其中Index同时用于音视频流是否存在的判断
    AVFormatContext* m_pFormatCtx;
    int m_videoIndex;
    int m_audioIndex;
    AVCodecContext* m_pCodecCtx_video;
    AVCodecContext* m_pCodecCtx_audio;
    const AVCodec* m_pCodec_video;
    const AVCodec* m_pCodec_audio;

    //视频变量
    uint8_t* m_frameBuf;
    AVFrame* m_frameRGB;
    SwsContext* m_pSwsCtx;
    bool m_RGBMode;
    double m_aspectRatio;
    int m_windowWidth;
    int m_windowHeight;

    //音频变量
    AudioParams* m_pAudioParams;
    SwrContext* m_swrCtx;

    //公共变量
    float m_speedFactor;

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

    //流媒体转换
    std::string m_inputStreamUrl;
    std::string m_outputStreamUrl;
};

#endif // MEDIAMANAGER_H
