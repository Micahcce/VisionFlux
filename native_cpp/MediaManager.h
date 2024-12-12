#ifndef MEDIAMANAGER_H
#define MEDIAMANAGER_H

#include <iostream>
#include <stdexcept>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include "MediaQueue.h"
#include "SdlPlayer.h"
#include "Logger.h"
#include "SystemClock.h"
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
    void seekFrameByStream(int timeSecs);

    //获取当前进度
    float getCurrentProgress() const;

    //获取流索引
    int getAudioIndex() const {return m_audioIndex;}
    int getVideoIndex() const {return m_videoIndex;}

    //设置音量
    void changeVolume(int volume);

    //变速
    void changeSpeed(float speedFactor);

    //尺寸调整
    void frameResize(int width, int height, bool uniformScale);

    //cuda加速
    void setSafeCudaAccelerate(bool state) {m_safeCudaAccelerate = state;}

    //线程状态
    bool getThreadSafeExited() {return m_thread_safe_exited;}
    void setThreadQuit(bool state) {m_thread_quit = state;}
    void setThreadPause(bool state)
    {
        m_thread_pause = state;
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

private:
    enum MediaQueueCapacity
    {
        //帧缓冲数量过少（如1、2）当连续解码音频或视频帧时，会使解码线程阻塞导致无法继续解码。
        //音频帧率通常为40多（采样率/每帧样本数）
        MAX_AUDIO_FRAMES = 30,
        MAX_VIDEO_FRAMES = 20,
        MAX_AUDIO_PACKETS = 30,
        MAX_VIDEO_PACKETS = 20
    };

    enum
    {
        MAX_AUDIO_FRAME_SIZE = 192000,       // 1 second of 48khz 32bit audio    //48000 * (32/8)
        TMP_BUFFER_NUMBER = 3               // 需要足够大小，和帧率成反比。
        /*Qt每秒重绘可达数十过百次，视频24帧即平均41.666ms更新一帧才会scale刷新帧大小，Qt重绘按每秒100次平均10ms重绘一次，即在两帧间隔内数据大小会变动4次(41/4)，此时需要4个缓冲区才不会造成数据溢出，
         * 而视频两帧间隔是不均匀的，也就是说需要提供更多的缓冲区。
         * 然而如果在渲染端增加一个缓冲区，此处则只需要三个缓冲区
         * Qt每秒重绘次数与屏幕刷新率以及CPU速度有关*/
    };

    void initVideoCodec();
    void initAudioCodec();
    void initAudioDevice();
    void frameYuvToRgb();
    void renderDelayControl(AVFrame* frame);
    void delayMs(int ms);
    void renderFrameRgb();

    //线程函数
    int thread_media_read();
    int thread_video_decode();
    int thread_audio_decode();
    int thread_video_display();
    int thread_audio_display();
    int thread_stream_convert();


    //线程状态变量
    std::atomic<bool> m_thread_quit;
    std::atomic<bool> m_thread_pause;
    bool m_thread_safe_exited;
    bool m_thread_media_read_exited;
    bool m_thread_video_decode_exited;
    bool m_thread_audio_decode_exited;
    bool m_thread_video_display_exited;
    bool m_thread_audio_display_exited;


    //回调，用于GUI渲染
    RenderCallback m_renderCallback;

    //自定义成员类
    MediaQueue* m_mediaQueue;
    SystemClock* m_systemClock;
    SdlPlayer* m_sdlPlayer;
    HANDLE m_soundTouch;

    //媒体数据相关，其中Index同时用于音视频流是否存在的判断
    AVFormatContext* m_formatCtx;
    int m_videoIndex;
    int m_audioIndex;
    AVCodecContext* m_videoCodecCtx;
    AVCodecContext* m_audioCodecCtx;
    const AVCodec* m_videoCodec;
    const AVCodec* m_audioCodec;

    //视频变量
    uint8_t* m_frameBuf;
    AVFrame* m_frame;
    AVFrame* m_frameSw;
    AVFrame* m_frameRgb;
    SwsContext* m_swsCtx;
    bool m_rgbMode;
    double m_aspectRatio;
    int m_windowWidth;
    int m_windowHeight;
    std::mutex m_renderMtx;
    bool m_cudaAccelerate;
    bool m_safeCudaAccelerate;
    AVBufferRef* m_deviceCtx;

    //音频变量
    SwrContext* m_swrCtx;
    unsigned char * m_outBuf;

    //公共变量
    std::mutex m_videoDecodeMtx;
    std::mutex m_audioDecodeMtx;
    float m_speedFactor;

    //最后一帧的PTS，已转换为秒数
    double m_videoLastPTS;
    double m_audioLastPTS;

    //流媒体转换
    std::string m_inputStreamUrl;
    std::string m_outputStreamUrl;
};

#endif // MEDIAMANAGER_H
