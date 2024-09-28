#include "MainWindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1000, 600);

    //媒体管理器
    m_playController = new PlayController;

    //设置回调
    m_playController->setRenderCallback(
                [this](AVFrame* frameRGB, int width, int height, float aspectRatio)
    {
        renderFrameRGB(frameRGB, width, height, aspectRatio);  // 渲染帧的回调
    });

    //播放窗口
    m_videoView = new QLabel(this);
    m_videoView->setStyleSheet("background-color:#FFFFFF;");
    m_videoView->show();
    m_videoView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    //播放列表
    m_sideBar = new SideBar(this);

    //底栏
    m_bottomBar = new ButtomBar(this);
    m_bottomBar->setPlayController(m_playController);       //传递MediaManager类
    m_bottomBar->setSideBar(m_sideBar);

    //布局管理器
    QVBoxLayout* vBox = new QVBoxLayout;
    vBox->addWidget(m_videoView);
    vBox->addWidget(m_bottomBar);

    QHBoxLayout* hBox = new QHBoxLayout;
    hBox->addLayout(vBox);
    hBox->addWidget(m_sideBar);

    QWidget* centralWidget = new QWidget(this);  // 创建一个中央 widget
    centralWidget->setLayout(hBox);  // 设置布局到 centralWidget
    setCentralWidget(centralWidget);  // 使用 setCentralWidget 设置中心窗口部件

    //初始化窗口宽高比
    m_videoView->adjustSize();
}

MainWindow::~MainWindow()
{
}

void MainWindow::renderFrameRGB(AVFrame *frameRGB, int width, int height, float aspectRatio)
{
    QImage img((uchar*)frameRGB->data[0], width, height, QImage::Format_RGB32);
    m_videoView->setPixmap(QPixmap::fromImage(img));
}



