#ifndef PLAYCONTROLLER_H
#define PLAYCONTROLLER_H

#include "MediaManager.h"
#include <string>

class MediaPlayInfo
{
public:
    bool isStarted = false;
    bool isPlaying = false;
};

class PlayController
{
public:
    PlayController();

    //开始播放
    void startPlay(std::string filePath);
    //继续播放
    void continuePlay();
    //暂停播放
    void pausePlay();
    //结束播放
    void endPlay();

    //获取视频时长（秒）
    int getMediaDuration(std::string filePath);

    //传递渲染回调
    void setRenderCallback(MediaManager::RenderCallback callback){m_mediaManager->setRenderCallback(std::move(callback));}

    //提供播放状态查询
    MediaPlayInfo* getMediaPlayInfo() {return m_mediaInfo;}

private:

    MediaPlayInfo* m_mediaInfo;
    MediaManager* m_mediaManager;
};

#endif // PLAYCONTROLLER_H
