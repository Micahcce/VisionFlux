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
    logger.debug("ready play: %s", filePath.data());
    m_mediaManager->decodeToPlay(filePath.data());

    //是否有音频流
    if(m_mediaManager->getAudioIndex() >= 0)
        m_mediaInfo->hasAudioStream = true;
    if(m_mediaManager->getVideoIndex() >= 0)
        m_mediaInfo->hasVideoStream = true;
    if(m_mediaInfo->hasAudioStream)
    {
        m_mediaManager->getSdlPlayer()->setVolume(m_mediaInfo->volume);
        m_mediaManager->audioChangeSpeed(m_mediaInfo->speed);
    }
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
    m_mediaInfo->hasAudioStream = false;
    m_mediaInfo->hasVideoStream = false;
}

void PlayController::changePlayProgress(int timeSecs)
{
    m_mediaManager->setThreadPause(true);
    if(m_mediaInfo->hasVideoStream == true)
        m_mediaManager->seekFrameByVideoStream(timeSecs);
    else
        m_mediaManager->seekFrameByAudioStream(timeSecs);
    m_mediaManager->setThreadPause(false);
}

void PlayController::changePlaySpeed(float speedFactor)
{
    if(m_mediaInfo->mediaName == "")
        return;

    m_mediaInfo->speed = speedFactor;

    if(m_mediaInfo->hasAudioStream)
        m_mediaManager->audioChangeSpeed(m_mediaInfo->speed);
    else
        ;//待完成：单视频流变速逻辑
}

void PlayController::changeVolume(int volume)
{
    if(m_mediaInfo->mediaName == "")
        return;

    m_mediaInfo->volume = volume;

    if(m_mediaInfo->hasAudioStream)
        m_mediaManager->getSdlPlayer()->setVolume(m_mediaInfo->volume);
}

int PlayController::getMediaDuration(std::string filePath)
{
    AVFormatContext* formatCtx  = m_mediaManager->getMediaInfo(filePath.data());
    if (!formatCtx)
        return 0; // 如果无法获取格式上下文，返回默认值
    int64_t duration = formatCtx->duration;  // 获取视频总时长（单位：微秒）
    avformat_close_input(&formatCtx);        // 释放资源
    int secs = duration / AV_TIME_BASE;      // 将微秒转换为秒

    return secs;
}

float PlayController::getPlayProgress()
{
    return m_mediaManager->getCurrentProgress();
}

bool PlayController::saveFrameToBmp(std::string filePath, std::string outputPath, int sec)
{
    if(m_mediaManager->saveFrameToBmp(filePath, outputPath, sec))
        return true;
    else
        return false;
}


std::string PlayController::timeFormatting(int secs)
{
    // 计算小时、分钟和秒
    int hours = secs / 3600;
    int minutes = (secs % 3600) / 60;
    int seconds = secs % 60;

    // 格式化为 HH:MM:SS
    char durationStr[9]; // 长度为 8 + 1（用于 '\0'）
    snprintf(durationStr, sizeof(durationStr), "%02d:%02d:%02d", hours, minutes, seconds);

    return std::string(durationStr); // 返回格式化后的字符串
}

