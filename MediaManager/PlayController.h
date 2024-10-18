#ifndef PLAYCONTROLLER_H
#define PLAYCONTROLLER_H

#include "MediaManager.h"
#include <string>
#include <istream>

class MediaPlayInfo
{
    //播放信息
public:
    bool isPlaying = false;
    bool isLiveStream = false;
    bool hasAudioStream = false;
    bool hasVideoStream = false;

    float speed = 1.0;
    int volume = 100;
    std::string mediaName = "";
};

class PlayController
{
public:
    PlayController();

    //开始播放
    void startPlay(const std::string filePath);
    //继续播放
    void continuePlay();
    //暂停播放
    void pausePlay();
    //结束播放
    void endPlay();

    //修改播放进度
    void changePlayProgress(int timeSecs);
    //修改播放速度
    void changePlaySpeed(float speedFactor);
    //修改音量
    void changeVolume(int volume);

    //推流
    void streamConvert(const std::string& inputStreamUrl, const std::string& outputStreamUrl);

    //获取视频时长（秒）
    int getMediaDuration(const std::string filePath);
    //获取当前播放进度
    float getPlayProgress();
    //保存图片
    bool saveFrameToBmp(const std::string filePath, const std::string outputPath, int sec);

    //格式化时长（hh:mm:ss）
    std::string timeFormatting(int secs);

    //传递渲染回调
    void setRenderCallback(MediaManager::RenderCallback callback){m_mediaManager->setRenderCallback(std::move(callback));}

    //提供播放状态查询
    const MediaPlayInfo* getMediaPlayInfo()
    {
        if (!m_mediaInfo)
        {
            std::cerr << "Media info is null." << std::endl;
            return nullptr;
        }
        return m_mediaInfo;
    }

private:

    MediaPlayInfo* m_mediaInfo;
    MediaManager* m_mediaManager;
};

#endif // PLAYCONTROLLER_H
