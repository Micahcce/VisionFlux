#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

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
#include <QFileDialog>
#include "PlayList.h"
#include "PlayController.h"
#include "ProcessPanel.h"


class BottomBar : public QWidget
{
    Q_OBJECT
public:
    BottomBar(QWidget *parent = nullptr);

    void setPlayController(PlayController* playController) {m_playController = playController;}
    void setPlayList(PlayList* playList);
    void setProcessPanel(ProcessPanel* processPanel);

public slots:
    void slotPlayAndPause();
    void slotAddMediaFile();
    void slotChangeSpeed();
    void slotVolumeChanged();

private slots:
    void slotUpdateProgress();
    bool slotStartPlayMedia();
    void slotSliderPressed();
    void slotSliderReleased();

private:
    PlayController* m_playController;
    PlayList* m_playList;
    ProcessPanel* m_processPanel;

    QLabel* m_currentTime;
    QSlider* m_timeSlider;
    QLabel* m_totalTime;

    QPushButton* m_playBtn;
    QVector<float> m_playbackSpeeds; // 存储播放速率
    int speedIndex;
    QPushButton* m_changeSpeedBtn;
    QPushButton* m_volumeBtn;
    QSlider* m_volumeSlider;
    QPushButton* m_addFileBtn;

    QTimer* m_sliderTimer;        //进度条定时器
};

#endif // BOTTOMBAR_H
