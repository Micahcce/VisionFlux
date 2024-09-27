#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include "MediaManager.h"

#include <QMainWindow>
#include <QLabel>
#include <QMediaPlayer>
#include <QTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QSlider>
#include <QPushButton>
#include <QApplication>
#include <QStyle>
#include <QDateTime>
#include <QDebug>


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

class VideoPlayInfo
{
public:
    bool isStarted = false;
    bool isPlaying = false;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString timeFormatting(int secs);       //秒数格式化为hh:mm:ss
    void renderFrameRGB(AVFrame *frameRGB, float aspectRatio);

public slots:
    void slotPlayVideo();
    void slotUpdateProgress();

private:
    MediaManager* m_mediaManeger;
    VideoPlayInfo* m_playInfo;

    QTimer* m_sliderTimer;        //进度条定时器 1000ms
    QDateTime m_lastTriggerTime = QDateTime::currentDateTime();  //上一次触发的时间
    QElapsedTimer* m_elapsedTimer;//校准定时器
    qint64 m_elapsedTime;         // 已经经过的时间
    bool m_needRectify = false;

///////////GUI控件///////////
    QLabel* m_videoView;
    QListWidget* m_playList;
    QWidget* m_bottomBar;

    //底栏中的控件
    QLabel* m_currentTime;
    QSlider* m_timeSlider;
    QLabel* m_totalTime;

    QPushButton* m_playBtn;
};
#endif // MAINWINDOW_H
