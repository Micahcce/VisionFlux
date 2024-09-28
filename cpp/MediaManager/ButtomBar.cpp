#include "ButtomBar.h"

ButtomBar::ButtomBar(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color:#DDEEDD;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(60);

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

bool ButtomBar::videoDoubleClicked()
{
    //结束当前视频
    m_playController->endPlay();

    //获取路径
    QString videoPath = m_sideBar->getVideoPath();
    if(videoPath == nullptr)
    {
        std::cerr << "video path get failed." << std::endl;
        return false;
    }

    //获取时长
    int totalTime = m_playController->getMediaDuration(videoPath.toStdString());
    QString totalTimeStr = timeFormatting(totalTime);

    // 设置相关控件
    m_timeSlider->setRange(0, totalTime);
    m_totalTime->setText(totalTimeStr);
    QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPause);
    m_playBtn->setIcon(playIcon);

    //开始播放
    m_playController->startPlay(videoPath.toStdString());

    return true;
}

void ButtomBar::slotPlayVideo()
{
    if(!m_playController->getMediaPlayInfo()->isStarted)    //未开始播放
    {
        if(videoDoubleClicked() == false)
            return;
    }
    else
    {
        if(m_playController->getMediaPlayInfo()->isPlaying)
        {
            //暂停
            m_playController->pausePlay();

            QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
            m_playBtn->setIcon(playIcon);

            // 记录误差时间
            m_elapsedTime = m_elapsedTimer->elapsed();
            m_needRectify = true;

            m_sliderTimer->stop();
        }
        else
        {
            //继续
            m_playController->continuePlay();

            QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPause);
            m_playBtn->setIcon(playIcon);

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
        m_playController->endPlay();
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
