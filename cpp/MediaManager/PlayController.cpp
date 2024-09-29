#include "PlayController.h"

PlayController::PlayController()
{
    m_mediaInfo = new MediaPlayInfo;
    m_mediaManager = new MediaManager;
}

void PlayController::startPlay(std::string filePath)
{
    while (m_mediaManager->getThreadSafeExited() == false)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cerr << "ready play: " << filePath << std::endl;
    m_mediaManager->decodeToPlay(filePath.data());
    m_mediaInfo->mediaName = filePath;
    m_mediaInfo->isStarted = true;
    m_mediaInfo->isPlaying = true;
}

void PlayController::continuePlay()
{
    m_mediaManager->setThreadPause(false);
    m_mediaInfo->isPlaying = true;
}

void PlayController::pausePlay()
{
    m_mediaManager->setThreadPause(true);
    m_mediaInfo->isPlaying = false;
}

void PlayController::endPlay()
{
    m_mediaManager->setThreadQuit(true);
    if(m_mediaInfo->mediaName != "")
        m_mediaManager->close();
    m_mediaInfo->mediaName = "";
    m_mediaInfo->isStarted = false;
    m_mediaInfo->isPlaying = false;
}

int PlayController::getMediaDuration(std::string filePath)
{
    AVFormatContext* formatCtx  = m_mediaManager->getMediaInfo(filePath.data());
    int64_t duration = formatCtx->duration;  // 获取视频总时长（单位：微秒）
    avformat_close_input(&formatCtx);        // 释放资源
    int secs = duration / AV_TIME_BASE;      // 将微秒转换为秒

    return secs;
}
