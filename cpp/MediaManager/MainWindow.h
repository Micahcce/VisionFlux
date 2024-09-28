#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include "PlayController.h"
#include "ButtomBar.h"
#include "PlayList.h"

#include <QMainWindow>
#include <QLabel>
#include <QMediaPlayer>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QPushButton>
#include <QTabWidget>
#include <QListWidget>


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

    void renderFrameRGB(AVFrame *frameRGB, int width, int height, float aspectRatio);

private:
    PlayController* m_playController;
    QLabel* m_videoView;
    PlayList* m_playList;
    ButtomBar* m_bottomBar;
};
#endif // MAINWINDOW_H
