#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

extern "C"
{
#include "libavcodec/avcodec.h"
}


class FrameQueue
{
public:
    FrameQueue() : m_exit(false) {}
    ~FrameQueue() { clear(); }

    // Add a method to signal exit
    void signalExit()
    {
        m_exit = true;
        cv.notify_all(); // Notify all waiting threads
    }

    // Reset the frame queue for reuse
    void reset()
    {
        m_exit = false;
        clear();
    }

    // Push an audio frame into the queue
    void pushAudioFrame(const AVFrame* frame)
    {
        AVFrame* audioFrame = av_frame_alloc();
        av_frame_ref(audioFrame, frame);

        std::unique_lock<std::mutex> lock(mutex);
        audioFrames.push(audioFrame);
        cv.notify_one();
    }

    // Push a video frame into the queue
    void pushVideoFrame(const AVFrame* frame)
    {
        AVFrame* videoFrame = av_frame_alloc();
        av_frame_ref(videoFrame, frame);

        std::unique_lock<std::mutex> lock(mutex);
        videoFrames.push(videoFrame);
        cv.notify_one();
    }

    // Pop an audio frame from the queue
    AVFrame* popAudioFrame()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (audioFrames.empty() && !m_exit)
        {
            cv.wait(lock);
        }
        if (m_exit) return nullptr; // Return nullptr if exit signaled
        AVFrame* frame = audioFrames.front();
        audioFrames.pop();
        return frame;
    }

    // Pop a video frame from the queue
    AVFrame* popVideoFrame()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (videoFrames.empty() && !m_exit)
        {
            cv.wait(lock);
        }
        if (m_exit) return nullptr; // Return nullptr if exit signaled
        AVFrame* frame = videoFrames.front();
        videoFrames.pop();
        return frame;
    }

    // Check if we should exit
    bool shouldExit() const { return m_exit; }

    int getAudioFrameCount() const { return audioFrames.size(); }
    int getVideoFrameCount() const { return videoFrames.size(); }

    void clear()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!audioFrames.empty())
        {
            av_frame_free(&audioFrames.front());
            audioFrames.pop();
        }
        while (!videoFrames.empty())
        {
            av_frame_free(&videoFrames.front());
            videoFrames.pop();
        }
    }

private:

    std::queue<AVFrame*> audioFrames; // Queue for audio frames
    std::queue<AVFrame*> videoFrames; // Queue for video frames
    std::mutex mutex;                   // Mutex for thread safety
    std::condition_variable cv;         // Condition variable for synchronization
    std::atomic<bool> m_exit;                        // Exit flag
};



#endif // FRAMEQUEUE_H
