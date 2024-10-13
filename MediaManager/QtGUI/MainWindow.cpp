#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_mediaDirPath("C:\\Users\\13055\\Desktop\\VisionFlux\\media")
{
    resize(1000, 600);

    //播放控制器
    m_playController = new PlayController;

    //设置回调
    m_playController->setRenderCallback(
                [this](uint8_t* data, int width, int height, float aspectRatio)
    {
        renderFrameRGB(data, width, height, aspectRatio);  // 渲染帧的回调
    });

    //播放窗口
    m_videoView = new QLabel(this);
    m_videoView->setStyleSheet("background-color:#FFFFFF;");
    m_videoView->show();
    m_videoView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    //侧边栏
    QTabWidget* sideBar = new QTabWidget(this);
    sideBar->setStyleSheet("background-color:#EEEEDD;");
    sideBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    sideBar->setFixedWidth(280);

    QFont font("微软雅黑");
    QWidget* playListTab = new QWidget(this);
    QWidget* videoSummaryTab = new QWidget(this);
    playListTab->resize(100, 40);
    videoSummaryTab->resize(100, 40);
    sideBar->setFont(font);
    sideBar->addTab(playListTab, "播放列表");
    sideBar->addTab(videoSummaryTab, "总结");
    sideBar->setStyleSheet("QTabBar::tab { width: 100px; height: 40px;background-color:transparent;color:rgb(120,120,120);}"
          "QTabBar::tab:selected{ color:rgb(75,75,110); border-bottom:2px solid#4b4b6e; }"
          "QTabBar::tab:hover{ color:rgb(0,0,0); }"
                  "QTabWidget:pane{border:0px; background-color:transparent}");


    //播放列表
    m_playList = new PlayList(playListTab);
    m_playList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout* vBox1 = new QVBoxLayout;
    vBox1->addWidget(m_playList);
    playListTab->setLayout(vBox1);

    //底栏
    m_bottomBar = new ButtomBar(this);
    m_bottomBar->setPlayController(m_playController);       //传递MediaManager类
    m_bottomBar->setPlayList(m_playList);

    //添加视频列表
    m_bottomBar->searchMediaFiles(m_mediaDirPath);

    //布局管理器
    QVBoxLayout* vBox = new QVBoxLayout;
    vBox->addWidget(m_videoView);
    vBox->addWidget(m_bottomBar);

    QHBoxLayout* hBox = new QHBoxLayout;
    hBox->addLayout(vBox);
    hBox->addWidget(sideBar);

    QWidget* centralWidget = new QWidget(this);  // 创建一个中央 widget
    centralWidget->setLayout(hBox);  // 设置布局到 centralWidget
    setCentralWidget(centralWidget);  // 使用 setCentralWidget 设置中心窗口部件

    //初始化窗口宽高比
    m_videoView->adjustSize();
}

MainWindow::~MainWindow()
{
}

void MainWindow::renderFrameRGB(uint8_t *data, int width, int height, float aspectRatio)
{
    QImage img((uchar*)data, width, height, QImage::Format_RGB32);
    m_videoView->setPixmap(QPixmap::fromImage(img));
}



