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
#include "SideBar.h"

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
    void setSideBar(SideBar* sideBar) {m_sideBar = sideBar;}
    void videoDoubleClicked();

public slots:
    void slotPlayVideo();
    void slotUpdateProgress();

private:
    QString timeFormatting(int secs);       //秒数格式化为hh:mm:ss

    VideoPlayInfo* m_playInfo;
    MediaManager* m_mediaManager;
    SideBar* m_sideBar;

    QLabel* m_currentTime;
    QSlider* m_timeSlider;
    QLabel* m_totalTime;

    QPushButton* m_playBtn;


    QTimer* m_sliderTimer;        //进度条定时器 1000ms
    QElapsedTimer* m_elapsedTimer;//校准定时器
    qint64 m_elapsedTime;         //已经经过的时间
    bool m_needRectify = false;

};

#endif // BUTTOMBAR_H
