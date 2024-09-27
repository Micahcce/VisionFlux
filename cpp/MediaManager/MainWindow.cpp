#include "MainWindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1000, 600);
    m_playInfo = new VideoPlayInfo;
    m_mediaManeger = new MediaManager;
    m_mediaManeger->setRenderCallback(
                [this](AVFrame* frameRGB, float aspectRatio)
    {
        renderFrameRGB(frameRGB, aspectRatio);  // 渲染帧的回调
    });

    //播放窗口
    m_videoView = new QLabel(this);
    m_videoView->setStyleSheet("background-color:#FFFFFF;");
    m_videoView->show();
    m_videoView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    //底栏
    m_bottomBar = new QWidget(this);
    m_bottomBar->setStyleSheet("background-color:#DDEEDD;");
    m_bottomBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_bottomBar->setFixedHeight(60);

    m_currentTime = new QLabel(this);
    m_currentTime->setText("00:00:00");
    m_timeSlider = new QSlider(Qt::Horizontal, this);
    m_timeSlider->setValue(0);
    m_totalTime = new QLabel(this);
    m_totalTime->setText("00:00:00");

    m_sliderTimer = new QTimer(this);
    connect(m_sliderTimer, &QTimer::timeout, this, &MainWindow::slotUpdateProgress);
    m_elapsedTimer = new QElapsedTimer;

    QHBoxLayout* bottomHBox = new QHBoxLayout;
    bottomHBox->addWidget(m_currentTime);
    bottomHBox->addWidget(m_timeSlider);
    bottomHBox->addWidget(m_totalTime);

    m_playBtn = new QPushButton(this);
    m_playBtn->setFixedSize(30, 30);
    QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
    m_playBtn->setIcon(playIcon);
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::slotPlayVideo);

    QVBoxLayout* bottomVBox = new QVBoxLayout;
    bottomVBox->addLayout(bottomHBox);
    bottomVBox->addWidget(m_playBtn);
    m_bottomBar->setLayout(bottomVBox);

    //播放列表
    m_playList = new QListWidget(this);
    m_playList->setStyleSheet("background-color:#EEEEDD;");
    m_playList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_playList->setFixedWidth(280);

    //布局管理器
    QVBoxLayout* vBox = new QVBoxLayout;
    vBox->addWidget(m_videoView);
    vBox->addWidget(m_bottomBar);

    QHBoxLayout* hBox = new QHBoxLayout;
    hBox->addLayout(vBox);
    hBox->addWidget(m_playList);

    QWidget* centralWidget = new QWidget(this);  // 创建一个中央 widget
    centralWidget->setLayout(hBox);  // 设置布局到 centralWidget
    setCentralWidget(centralWidget);  // 使用 setCentralWidget 设置中心窗口部件

    //初始化窗口宽高比
    m_videoView->adjustSize();
}

MainWindow::~MainWindow()
{
}

QString MainWindow::timeFormatting(int secs)
{
    // 计算小时、分钟、秒
    int hours = secs / 3600;
    int minutes = (secs % 3600) / 60;
    int seconds = secs % 60;

    QString totalTimeStr = QString("%1:%2:%3")
                            .arg(hours, 2, 10, QChar('0'))   // 小时，两位数字
                            .arg(minutes, 2, 10, QChar('0')) // 分钟，两位数字
                            .arg(seconds, 2, 10, QChar('0'));   // 秒，两位数字

    return totalTimeStr;
}

void MainWindow::renderFrameRGB(AVFrame *frameRGB, float aspectRatio)
{
    int width = 388;
    int height = 218;

    QImage img((uchar*)frameRGB->data[0], width, height, QImage::Format_RGB32);
    m_videoView->setPixmap(QPixmap::fromImage(img));
}

void MainWindow::slotPlayVideo()
{
    //未开始则播放
    if(!m_playInfo->isStarted)
    {
        m_playInfo->isStarted = true;

        //获取时长
        AVFormatContext* formatCtx  = m_mediaManeger->getMediaInfo("C:\\Users\\13055\\Desktop\\aki.mp4");
        int64_t duration = formatCtx->duration;  // 获取视频总时长（单位：微秒）
        avformat_close_input(&formatCtx);        // 释放资源
        int secs = duration / AV_TIME_BASE;     // 将微秒转换为秒
        QString totalTimeStr = timeFormatting(secs);    //格式化

        // 设置相关控件
        m_totalTime->setText(totalTimeStr);
        m_timeSlider->setRange(0, secs);

        // 播放
        m_mediaManeger->decodeToPlay("C:\\Users\\13055\\Desktop\\aki.mp4");
    }

    //播放状态切换
    if(m_playInfo->isPlaying)
    {
        m_playInfo->isPlaying = false;
        QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
        m_playBtn->setIcon(playIcon);
        m_mediaManeger->setThreadPause(true);

        // 记录误差时间
        m_elapsedTime = m_elapsedTimer->elapsed();
        m_needRectify = true;

        m_sliderTimer->stop();
    }
    else
    {
        m_playInfo->isPlaying = true;
        QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPause);
        m_playBtn->setIcon(playIcon);
        m_mediaManeger->setThreadPause(false);

        if(m_needRectify)           // 校正误差时间
        {
            int remainingTime = 1000 - m_elapsedTime % 1000;  // 计算剩余时间
            std::cout << remainingTime << std::endl;
            m_sliderTimer->start(remainingTime);       // 重新启动定时器，剩余时间作为间隔
        }
        else
        {
            m_sliderTimer->start(1000);
            m_elapsedTimer->start();
        }
    }
}

void MainWindow::slotUpdateProgress()
{
    //进度条增加
    int currentValue = m_timeSlider->value();
    if (currentValue < m_timeSlider->maximum())
    {
        m_timeSlider->setValue(currentValue + 1);
    }

    //当前时长增加
    int currentTime = m_timeSlider->value();
    QString currentTimeStr = timeFormatting(currentTime);
    m_currentTime->setText(currentTimeStr);

    //播放完成
    if(currentTime == m_timeSlider->maximum())
    {
        m_sliderTimer->stop();
        m_mediaManeger->setThreadQuit(true);
    }

    //开启误差计时器
    m_elapsedTimer->start();

    //校正完成，恢复定时间隔
    if(m_needRectify)
    {
        m_needRectify = false;
        m_sliderTimer->start(1000);
    }
}


