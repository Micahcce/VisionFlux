#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include "MediaManager.h"
#include "ButtomBar.h"
#include "SideBar.h"

#include <QMainWindow>
#include <QLabel>
#include <QMediaPlayer>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QPushButton>
#include <QTabWidget>


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void renderFrameRGB(AVFrame *frameRGB, float aspectRatio);

private:
    MediaManager* m_mediaManeger;
    QLabel* m_videoView;
    SideBar* m_sideBar;
    ButtomBar* m_bottomBar;
};
#endif // MAINWINDOW_H
