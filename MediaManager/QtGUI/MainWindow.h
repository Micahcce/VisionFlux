#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include "PlayController.h"
#include "BottomBar.h"
#include "PlayList.h"
#include "ProcessPanel.h"

#include <QMainWindow>
#include <QLabel>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QPushButton>
#include <QTabWidget>
#include <QListWidget>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void renderFrameRGB(uint8_t *data, int width, int height, float aspectRatio);

private:
    PlayController* m_playController;
    QLabel* m_videoView;
    PlayList* m_playList;
    ProcessPanel* m_processPanel;
    BottomBar* m_bottomBar;

    QString m_mediaDirPath;
};
#endif // MAINWINDOW_H