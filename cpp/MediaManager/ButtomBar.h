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
#include "PlayList.h"
#include "PlayController.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}


class ButtomBar : public QWidget
{
    Q_OBJECT
public:
    ButtomBar(QWidget *parent = nullptr);

    void setPlayController(PlayController* playController) {m_playController = playController;}
    void setPlayList(PlayList* playList);

public slots:
    void slotPlayVideo();
    void slotUpdateProgress();
    bool slotVideoDoubleClicked();

private:
    QString timeFormatting(int secs);       //秒数格式化为hh:mm:ss
    PlayController* m_playController;
    PlayList* m_playList;

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
