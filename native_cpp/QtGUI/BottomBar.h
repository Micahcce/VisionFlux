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
#include <QShortcut>
#include <QCheckBox>
#include <QCameraInfo>
#include "PlayController.h"


class BottomBar : public QWidget
{
    Q_OBJECT
public:
    BottomBar(QWidget *parent = nullptr);

    void setPlayController(PlayController* playController) {m_playController = playController;}
    void changeProgress(int changeSecs);

signals:
    void sigStartPlayMedia();
    void sigAddMediaItem(QString filePath);

public slots:
    bool slotStartPlayMedia(QString mediaPath, bool cameraInput = false);

private slots:
    void slotPlayAndPause();
    void slotAddMediaFile();
    void slotChangeSpeed();
    void slotVolumeChanged();
    void slotUpdateProgress();
    void slotSliderPressed();
    void slotSliderReleased();
    void slotOpenCamera();

private:
    enum
    {
        TIMING_INTERVAL = 200
    };

    PlayController* m_playController;

    QLabel* m_currentTime;
    QSlider* m_timeSlider;
    QLabel* m_totalTime;

    QPushButton* m_playBtn;
    QVector<float> m_playbackSpeeds; // 存储播放速率
    int speedIndex;
    QPushButton* m_changeSpeedBtn;
    QPushButton* m_volumeBtn;
    QSlider* m_volumeSlider;
    QCheckBox* m_hwAccelerateCb;
    QPushButton* m_addFileBtn;
    QTimer* m_sliderTimer;        //进度条定时器
    QPushButton* m_cameraBtn;
};

#endif // BOTTOMBAR_H
