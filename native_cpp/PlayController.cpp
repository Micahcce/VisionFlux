#include "PlayController.h"

PlayController::PlayController()
{
    m_allowedExtensions = "*.mp3 *.mp4 *.wav *.m4s *.ts *.flv *.mkv";

    m_mediaInfo = new MediaPlayInfo;
    m_mediaManager = new MediaManager;
}

void PlayController::startPlay(const std::string filePath)
{
    while (m_mediaManager->getThreadSafeExited() == false)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::ifstream file(filePath);
    if(file.good())                // 如果文件存在且可访问
        m_mediaInfo->isLiveStream = false;
    else
        m_mediaInfo->isLiveStream = true;

    logger.debug("ready play: %s", filePath.data());
    if(m_mediaManager->decodeToPlay(filePath) == false)
    {
        logger.error("media play failed.");
        return;
    }

    // 是否有音频流
    if(m_mediaManager->getAudioIndex() >= 0)
        m_mediaInfo->hasAudioStream = true;
    if(m_mediaManager->getVideoIndex() >= 0)
        m_mediaInfo->hasVideoStream = true;
    if(m_mediaInfo->hasAudioStream)
    {
        m_mediaManager->changeVolume(m_mediaInfo->volume);
    }
    m_mediaManager->changeSpeed(m_mediaInfo->speed);
    m_mediaInfo->mediaName = filePath;
    m_mediaInfo->isPlaying = true;
}

void PlayController::resumePlay()
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
    m_mediaInfo->isPlaying = false;
    m_mediaInfo->hasAudioStream = false;
    m_mediaInfo->hasVideoStream = false;
    m_mediaInfo->isLiveStream = false;
}

void PlayController::changePlayProgress(int timeSecs)
{
    m_mediaManager->setThreadPause(true);
    m_mediaManager->seekFrameByStream(timeSecs);
    if(m_mediaInfo->isPlaying)
        m_mediaManager->setThreadPause(false);
}

void PlayController::changePlaySpeed(float speedFactor)
{
    m_mediaInfo->speed = speedFactor;

    if(m_mediaInfo->mediaName == "")
        return;

    m_mediaManager->changeSpeed(m_mediaInfo->speed);
}

void PlayController::changeVolume(int volume)
{
    if(m_mediaInfo->mediaName == "")
        return;

    m_mediaInfo->volume = volume;

    if(m_mediaInfo->hasAudioStream)
        m_mediaManager->changeVolume(m_mediaInfo->volume);
}

void PlayController::changeFrameSize(int width, int height, bool uniformScale)
{
    if(m_mediaInfo->mediaName == "")
        return;
    if(m_mediaInfo->hasVideoStream == false)
        return;
    m_mediaManager->frameResize(width, height, uniformScale);
}

float PlayController::getPlayProgress()
{
    return m_mediaManager->getCurrentProgress();
}

void PlayController::setSafeCudaAccelerate(bool state)
{
#ifndef CUDA_ISAVAILABLE
    logger.warning("cuda is not available.");
    return;
#endif

    if(state)
        logger.info("cuda accelerate enabled, cold swapping");
    else
        logger.info("cuda accelerate disabled, cold swapping");

    m_mediaManager->setSafeCudaAccelerate(state);
}

void PlayController::streamConvert(const std::string& inputStreamUrl, const std::string& outputStreamUrll)
{
    m_mediaManager->streamConvert(inputStreamUrl, outputStreamUrll);
}

int PlayController::getMediaDuration(const std::string& filePath)
{
    return uGetMediaDuration(filePath);
}

bool PlayController::saveFrameToBmp(const std::string& filePath, const std::string& outputPath, int sec)
{
    return uSaveFrameToBmp(filePath, outputPath, sec);
}

std::string PlayController::timeFormatting(int secs)
{
    return uTimeFormatting(secs);
}


