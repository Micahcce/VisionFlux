#include "ButtomBar.h"

ButtomBar::ButtomBar(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background-color:#DDEEDD;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(60);

//////////上排//////////
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

//////////下排//////////
    //播放
    m_playBtn = new QPushButton(this);
    m_playBtn->setFixedSize(30, 30);
    QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
    m_playBtn->setIcon(playIcon);
    connect(m_playBtn, &QPushButton::clicked, this, &ButtomBar::slotPlayVideo);

    //音量
    m_volumeBtn = new QPushButton(this);
    m_volumeBtn->setFixedSize(30, 30);
    QIcon volumeIcon = QApplication::style()->standardIcon(QStyle::SP_MediaVolume);
    m_volumeBtn->setIcon(volumeIcon);
    connect(m_volumeBtn, &QPushButton::clicked, this, &ButtomBar::slotVolumeChanged);

    m_volumeSlider = new QSlider(Qt::Vertical, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50); // 设置默认音量
    m_volumeSlider->setVisible(false);
    m_volumeSlider->setFixedSize(30, 100); // 设置滑块大小

    m_volumeBtn->installEventFilter(this);   // 监听鼠标进入和离开事件
    m_volumeSlider->installEventFilter(this); // 监听鼠标进入和离开事件

    //添加文件
    m_addFileBtn = new QPushButton(this);
    m_addFileBtn->setFixedSize(30, 30);
    QIcon addIcon = QApplication::style()->standardIcon(QStyle::SP_FileDialogStart);
    m_addFileBtn->setIcon(addIcon);
    connect(m_addFileBtn, &QPushButton::clicked, this, &ButtomBar::slotAddFile);

    QHBoxLayout* hBox2 = new QHBoxLayout;
    hBox2->addWidget(m_playBtn);
    hBox2->addWidget(m_volumeBtn);
    hBox2->addWidget(m_addFileBtn);

    //垂直布局
    QVBoxLayout* vBox = new QVBoxLayout;
    vBox->addLayout(hBox);
    vBox->addLayout(hBox2);

    this->setLayout(vBox);
}

void ButtomBar::setPlayList(PlayList *playList)
{
    m_playList = playList;
    connect(m_playList, &QListWidget::itemDoubleClicked, this, &ButtomBar::slotVideoDoubleClicked);  //双击播放
}

bool ButtomBar::slotVideoDoubleClicked()
{
    //获取路径
    QString videoPath = m_playList->getVideoPath();
    if(videoPath.toStdString() == m_playController->getMediaPlayInfo()->mediaName)
    {
        return false;
    }
    else if(videoPath == "")
    {
        std::cerr << "video path get failed." << std::endl;
        return false;
    }

    //结束当前视频
    m_playController->endPlay();

    //获取时长
    int totalTime = m_playController->getMediaDuration(videoPath.toStdString());
    QString totalTimeStr = QString::fromStdString(m_playController->timeFormatting(totalTime));

    // 设置相关控件
    m_timeSlider->setRange(0, totalTime);
    m_timeSlider->setValue(0);
    m_currentTime->setText("00:00:00");
    m_totalTime->setText(totalTimeStr);
    QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPause);
    m_playBtn->setIcon(playIcon);

    //开始播放
    m_playController->startPlay(videoPath.toStdString());

    //定时器启动
    m_sliderTimer->start(1000);
    m_elapsedTimer->start();

    return true;
}

bool ButtomBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_volumeBtn)
    {
        if (event->type() == QEvent::Enter)
        {
            logger.debug("btn enter");
            return true;
        }
        else if (event->type() == QEvent::Leave)
        {
            logger.debug("btn leave");
            return true;
        }
    }
    else if (obj == m_volumeSlider)
    {
        if (event->type() == QEvent::Enter)
        {
            logger.debug("slider enter");
            return true;
        }
        else if (event->type() == QEvent::Leave)
        {
            logger.debug("slider leave");
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ButtomBar::slotPlayVideo()
{
    if(!m_playController->getMediaPlayInfo()->isStarted)    //未开始播放
    {
        if(slotVideoDoubleClicked() == false)
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

void ButtomBar::slotAddFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择媒体文件", "./", "*.mp4 *.wav");
    if(filePath == "")
        return;
    m_playList->addVideoItem(filePath, "10", "...");
}

void ButtomBar::slotVolumeChanged()
{

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
    QString currentTimeStr = QString::fromStdString(m_playController->timeFormatting(currentTime));
    m_currentTime->setText(currentTimeStr);

    //播放完成
    if(currentTime == m_timeSlider->maximum())
    {
        m_playController->endPlay();
        QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
        m_playBtn->setIcon(playIcon);
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
