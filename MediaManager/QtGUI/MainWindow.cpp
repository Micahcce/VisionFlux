#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_mediaDirPath("../media/")
{
    resize(1000, 600);

    //播放控制器
    m_playController = new PlayController;

    //设置回调
    m_playController->setRenderCallback(
                [this](uint8_t* data, int width, int height)
    {
        emitRenderSignal(data, width, height);  // 渲染帧的回调
    });

    //连接渲染信号
    connect(this, &MainWindow::sigRender, this, &MainWindow::renderFrameRGB);

    //播放窗口
    m_videoView = new QLabel(this);
    m_videoView->setStyleSheet("background-color:#FFFFFF;");
    m_videoView->show();
    m_videoView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    //底栏
    m_bottomBar = new BottomBar(this);
    m_bottomBar->setPlayController(m_playController);

    //侧边栏
    QTabWidget* sideBar = new QTabWidget(this);
    sideBar->setStyleSheet("background-color:#EEEEDD;");
    sideBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    sideBar->setFixedWidth(300);

    QFont font("微软雅黑");
    QWidget* playListTab = new QWidget(this);
    QWidget* processPanelTab = new QWidget(this);
    QWidget* videoSummaryTab = new QWidget(this);
    sideBar->setFont(font);
    sideBar->addTab(playListTab, "播放列表");
    sideBar->addTab(processPanelTab, "处理");
    sideBar->addTab(videoSummaryTab, "总结");
    sideBar->setStyleSheet("QTabBar::tab { width: 100px; height: 40px;background-color:transparent;color:rgb(120,120,120);}"
          "QTabBar::tab:selected{ color:rgb(75,75,110); border-bottom:2px solid#4b4b6e; }"
          "QTabBar::tab:hover{ color:rgb(0,0,0); }"
                  "QTabWidget:pane{border:0px; background-color:transparent}");


    //播放列表
    m_playList = new PlayList(playListTab);
    m_playList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_playList->setBottomBar(m_bottomBar);
    m_playList->setMediaDirPath(m_mediaDirPath);
    m_playList->searchMediaFiles();           //添加视频列表

    QVBoxLayout* vBox1 = new QVBoxLayout;
    vBox1->addWidget(m_playList);
    playListTab->setLayout(vBox1);

    //流处理面板
    m_processPanel = new ProcessPanel(processPanelTab);
    m_processPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_processPanel->setBottomBar(m_bottomBar);

    QVBoxLayout* vBox2 = new QVBoxLayout;
    vBox2->addWidget(m_processPanel);
    processPanelTab->setLayout(vBox2);

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

void MainWindow::emitRenderSignal(uint8_t *data, int width, int height)
{
    emit sigRender(data, width, height);
}

void MainWindow::renderFrameRGB(uint8_t *data, int width, int height)
{
    QImage img((uchar*)data, width, height, QImage::Format_RGB32);
    m_videoView->setPixmap(QPixmap::fromImage(img));
}



