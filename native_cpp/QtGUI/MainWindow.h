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
#include <QScreen>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void emitRenderSignal(uint8_t *data, int width, int height);
    void renderFrameRgb(uint8_t *data, int width, int height);

signals:
    void sigRender(uint8_t *data, int width, int height);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    PlayController* m_playController;
    QLabel* m_videoView;
    PlayList* m_playList;
    ProcessPanel* m_processPanel;
    BottomBar* m_bottomBar;

    QStringList m_allowedExtensions;
    QString m_mediaDirPath;
    QPixmap m_pix;
};
#endif // MAINWINDOW_H
