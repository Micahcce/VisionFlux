#include "ButtomBar.h"

ButtomBar::ButtomBar(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color:#DDEEDD;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(60);

    m_playInfo = new VideoPlayInfo;

    m_currentTime = new QLabel(this);
    m_currentTime->setText("00:00:00");
    m_timeSlider = new QSlider(Qt::Horizontal, this);
    m_timeSlider->setValue(0);
    m_totalTime = new QLabel(this);
    m_totalTime->setText("00:00:00");

    m_sliderTimer = new QTimer(this);
    connect(m_sliderTimer, &QTimer::timeout, this, &ButtomBar::slotUpdateProgress);
    m_elapsedTimer = new QElapsedTimer;

    QHBoxLayout* hBox = new QHBoxLayout;
    hBox->addWidget(m_currentTime);
    hBox->addWidget(m_timeSlider);
    hBox->addWidget(m_totalTime);

    m_playBtn = new QPushButton(this);
    m_playBtn->setFixedSize(30, 30);
    QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
    m_playBtn->setIcon(playIcon);
    connect(m_playBtn, &QPushButton::clicked, this, &ButtomBar::slotPlayVideo);

    QVBoxLayout* vBox = new QVBoxLayout;
    vBox->addLayout(hBox);
    vBox->addWidget(m_playBtn);

    this->setLayout(vBox);
}

void ButtomBar::videoDoubleClicked()
{
    //结束当前视频
    if(m_playInfo->isStarted)
    {
        m_sliderTimer->stop();
        m_mediaManager->setThreadQuit(true);
        m_playInfo->isStarted = false;
    }

    //开始播放
    if(!m_playInfo->isStarted)
    {
        //获取视频路径
        QString videoPath = m_sideBar->getVideoPath();

        //获取时长
        AVFormatContext* formatCtx  = m_mediaManager->getMediaInfo(videoPath.toUtf8().data());
        int64_t duration = formatCtx->duration;  // 获取视频总时长（单位：微秒）
        avformat_close_input(&formatCtx);        // 释放资源
        int secs = duration / AV_TIME_BASE;     // 将微秒转换为秒
        QString totalTimeStr = timeFormatting(secs);    //格式化

        // 设置相关控件
        m_totalTime->setText(totalTimeStr);
        m_timeSlider->setRange(0, secs);

        // 播放
        m_mediaManager->decodeToPlay(videoPath.toUtf8().data());

        m_playInfo->isStarted = true;
    }
}

void ButtomBar::slotPlayVideo()
{
    //未开始则播放
    if(!m_playInfo->isStarted)
    {
        //获取视频路径
        QString videoPath = m_sideBar->getVideoPath();

        //获取时长
        AVFormatContext* formatCtx  = m_mediaManager->getMediaInfo(videoPath.toUtf8().data());
        int64_t duration = formatCtx->duration;  // 获取视频总时长（单位：微秒）
        avformat_close_input(&formatCtx);        // 释放资源
        int secs = duration / AV_TIME_BASE;     // 将微秒转换为秒
        QString totalTimeStr = timeFormatting(secs);    //格式化

        // 设置相关控件
        m_totalTime->setText(totalTimeStr);
        m_timeSlider->setRange(0, secs);

        // 播放
        m_mediaManager->decodeToPlay(videoPath.toUtf8().data());

        m_playInfo->isStarted = true;
    }

    //播放状态切换
    if(m_playInfo->isPlaying)
    {
        m_playInfo->isPlaying = false;
        QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
        m_playBtn->setIcon(playIcon);
        m_mediaManager->setThreadPause(true);

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
        m_mediaManager->setThreadPause(false);

        if(m_needRectify)           // 校正误差时间
        {
            int remainingTime = 1000 - m_elapsedTime % 1000;  // 计算剩余时间
            m_sliderTimer->start(remainingTime);              // 重新启动定时器，剩余时间作为间隔
        }
        else
        {
            m_sliderTimer->start(1000);
            m_elapsedTimer->start();
        }
    }
}

void ButtomBar::slotUpdateProgress()
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
        m_mediaManager->setThreadQuit(true);
        m_playInfo->isStarted = false;
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

QString ButtomBar::timeFormatting(int secs)
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
