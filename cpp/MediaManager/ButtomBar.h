#ifndef BUTTOMBAR_H
#define BUTTOMBAR_H

#include <iostream>
#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QApplication>
#include "MediaManager.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}


class VideoPlayInfo
{
public:
    bool isStarted = false;
    bool isPlaying = false;
};


class ButtomBar : public QWidget
{
    Q_OBJECT
public:
    ButtomBar(QWidget *parent = nullptr);

    void setMediaManager(MediaManager* mediaManager) {m_mediaManager = mediaManager;}
    QString timeFormatting(int secs);       //秒数格式化为hh:mm:ss

public slots:
    void slotPlayVideo();
    void slotUpdateProgress();

private:
    VideoPlayInfo* m_playInfo;
    MediaManager* m_mediaManager;

    QLabel* m_currentTime;
    QSlider* m_timeSlider;
    QLabel* m_totalTime;

    QPushButton* m_playBtn;


    QTimer* m_sliderTimer;        //进度条定时器 1000ms
    QDateTime m_lastTriggerTime = QDateTime::currentDateTime();  //上一次触发的时间
    QElapsedTimer* m_elapsedTimer;//校准定时器
    qint64 m_elapsedTime;         //已经经过的时间
    bool m_needRectify = false;

};

#endif // BUTTOMBAR_H
