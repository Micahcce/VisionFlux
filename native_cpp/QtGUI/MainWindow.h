#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <iostream>
#include "PlayController.h"
#include "BottomBar.h"
#include "PlayList.h"
#include "ProcessPanel.h"
#include "OpenGLWidget.h"

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

//#define OPENGL_RENDER

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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    PlayController* m_playController;
    QLabel* m_videoView;
    PlayList* m_playList;
    ProcessPanel* m_processPanel;
    BottomBar* m_bottomBar;
    OpenGLWidget* m_openglWidget;

    QString m_mediaDirPath;
    QPixmap m_pix;

    //窗口移动
    bool m_isDragging;
    QPoint m_lastMousePos;
};
#endif // MAINWINDOW_H
