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
    connect(this, &MainWindow::sigRender, this, &MainWindow::renderFrameRgb);

    //播放窗口
#ifdef OPENGL_RENDER
    //opengl
    m_openglWidget = new OpenGLWidget(this);
    m_openglWidget->setStyleSheet("background-color:#FFFFFF;");
    m_openglWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
#else
    //QLabel
    m_videoView = new QLabel(this);
    m_videoView->setStyleSheet("background-color:#FFFFFF;");
    m_videoView->show();
    m_videoView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_videoView->setAlignment(Qt::AlignCenter);
#endif

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
    m_playList->setPlayController(m_playController);
    m_playList->setMediaDirPath(m_mediaDirPath);
    m_playList->searchMediaFiles();           //添加视频列表

    QVBoxLayout* vBox1 = new QVBoxLayout;
    vBox1->addWidget(m_playList);
    playListTab->setLayout(vBox1);

    //流处理面板
    m_processPanel = new ProcessPanel(processPanelTab);
    m_processPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_processPanel->setPlayController(m_playController);

    QVBoxLayout* vBox2 = new QVBoxLayout;
    vBox2->addWidget(m_processPanel);
    processPanelTab->setLayout(vBox2);

    //布局管理器
    QVBoxLayout* vBox = new QVBoxLayout;
#ifdef OPENGL_RENDER
    vBox->addWidget(m_openglWidget);
#else
    vBox->addWidget(m_videoView);
#endif
    vBox->addWidget(m_bottomBar);

    QHBoxLayout* hBox = new QHBoxLayout;
    hBox->addLayout(vBox);
    hBox->addWidget(sideBar);

    QWidget* centralWidget = new QWidget(this);  // 创建一个中央 widget
    centralWidget->setLayout(hBox);  // 设置布局到 centralWidget
    setCentralWidget(centralWidget);  // 使用 setCentralWidget 设置中心窗口部件

#ifndef OPENGL_RENDER
    //初始化窗口宽高比
    m_videoView->adjustSize();
#endif

    //连接信号
    connect(m_bottomBar, &BottomBar::sigStartPlayMedia, m_playList, [this](){emit m_playList->sigPlayMedia(m_playList->getMediaPath());});  //点击播放图标发送播放信号
    connect(m_playList, &QListWidget::itemDoubleClicked, m_playList, [this](){emit m_playList->sigPlayMedia(m_playList->getMediaPath());}); //双击列表项目发送播放信号
    connect(m_playList, &PlayList::sigPlayMedia, m_bottomBar, &BottomBar::slotStartPlayMedia);               //播放信号连接播放函数
    connect(m_processPanel, &ProcessPanel::sigLiveStreamPlay, m_bottomBar, &BottomBar::slotStartPlayMedia);  //播放信号连接播放函数
    connect(m_bottomBar, &BottomBar::sigAddMediaItem, m_playList, &PlayList::slotAddMediaItem);              //添加播放项
}

MainWindow::~MainWindow()
{
}

void MainWindow::emitRenderSignal(uint8_t *data, int width, int height)
{
    emit sigRender(data, width, height);
}

void MainWindow::renderFrameRgb(uint8_t *data, int width, int height)
{
    //QLabel手动
//    static QScreen *screen = QGuiApplication::primaryScreen();
//    static QRect screenGeometry = screen->geometry();               // 逻辑分辨率
//    static qreal pixelRatio = screen->devicePixelRatio();           // 缩放因子
//    static QSize physicalSize = screenGeometry.size() * pixelRatio; // 物理分辨率
//    static uint8_t* buf = (uint8_t* )malloc(physicalSize.width() * physicalSize.height() * 4);
//    std::copy(data, data + (width * height * 4), buf);
//    QImage img((uchar*)buf, width, height, QImage::Format_RGB32);
//    m_videoView->setPixmap(QPixmap::fromImage(img));

#ifdef OPENGL_RENDER
    //QOpenGLWidget
    m_openglWidget->setImageData(data, width, height);
#else
    //QLabel自动
    QImage img((uchar*)data, width, height, QImage::Format_RGB32);
    m_pix = QPixmap::fromImage(img);
    QPixmap fitpix = m_pix.scaled(m_videoView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_videoView->setPixmap(fitpix);
#endif
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);  // 调用父类的 resizeEvent
#ifndef OPENGL_RENDER
    //QLabel自动
    if(m_pix.isNull() == false)
    {
        QPixmap fitpix = m_pix.scaled(m_videoView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_videoView->setPixmap(fitpix);

    //QLabel手动
//    m_playController->changeFrameSize(m_videoView->width(), m_videoView->height(), true);
    }
#endif
}

//鼠标点击事件，识别鼠标左键按下操作，并记录当前位置
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_lastMousePos = event->pos();
        m_isDragging = true;
    }
}

//鼠标移动事件，计算鼠标光标的坐标变化，并借助move事件使界面进行相同的移动。如果鼠标没有松开，需要判定鼠标一直在按钮内
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton))
        move(pos() + event->pos() - m_lastMousePos);        // 计算窗口的新位置并移动窗口
}

//鼠标释放事件
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_isDragging = false;               // 鼠标释放，停止拖动
}
