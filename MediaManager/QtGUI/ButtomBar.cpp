#include "ButtomBar.h"

ButtomBar::ButtomBar(QWidget *parent) : QWidget(parent), speedIndex(2)
{
    setStyleSheet("background-color:#DDEEDD;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(60);

//////////上排//////////
    m_currentTime = new QLabel(this);
    m_currentTime->setText("00:00:00");
    m_timeSlider = new QSlider(Qt::Horizontal, this);
    m_timeSlider->setValue(0);
    connect(m_timeSlider, &QSlider::sliderPressed, this, &ButtomBar::slotSliderPressed);
    connect(m_timeSlider, &QSlider::sliderReleased, this, &ButtomBar::slotSliderReleased);
    m_totalTime = new QLabel(this);
    m_totalTime->setText("00:00:00");

    m_sliderTimer = new QTimer(this);
    connect(m_sliderTimer, &QTimer::timeout, this, &ButtomBar::slotUpdateProgress);

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

    //倍速
    m_playbackSpeeds << 0.5 << 0.75 << 1 << 1.25 << 1.5 << 2;
    m_changeSpeedBtn = new QPushButton(this);
    m_changeSpeedBtn->setFixedSize(30, 30);
    m_changeSpeedBtn->setText("倍速");
    connect(m_changeSpeedBtn, &QPushButton::clicked, this, &ButtomBar::slotChangeSpeed);

    //音量
    m_volumeBtn = new QPushButton(this);
    m_volumeBtn->setFixedSize(30, 30);
    QIcon volumeIcon = QApplication::style()->standardIcon(QStyle::SP_MediaVolume);
    m_volumeBtn->setIcon(volumeIcon);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100); // 设置默认音量
    m_volumeSlider->setFixedSize(100, 10); // 设置滑块大小
    connect(m_volumeSlider, &QSlider::sliderMoved, this, &ButtomBar::slotVolumeChanged);

    //添加文件
    m_addFileBtn = new QPushButton(this);
    m_addFileBtn->setFixedSize(30, 30);
    QIcon addIcon = QApplication::style()->standardIcon(QStyle::SP_FileDialogStart);
    m_addFileBtn->setIcon(addIcon);
    connect(m_addFileBtn, &QPushButton::clicked, this, &ButtomBar::slotAddMediaFile);

    QHBoxLayout* hBox2 = new QHBoxLayout;
    hBox2->addWidget(m_playBtn);
    hBox2->addWidget(m_changeSpeedBtn);
    hBox2->addWidget(m_volumeBtn);
    hBox2->addWidget(m_volumeSlider);
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

void ButtomBar::setProcessPanel(ProcessPanel *processPanel)
{
    m_processPanel = processPanel;
}

void ButtomBar::searchMediaFiles(const QString &directoryPath)
{
    // 定义要搜索的文件扩展名
    QStringList filters;
    filters << "*.mp3" << "*.mp4" << "*.wav";

    // 创建 QDirIterator 以递归方式搜索文件
    QDirIterator it(directoryPath, filters, QDir::Files, QDirIterator::Subdirectories);

    // 迭代找到的文件
    while (it.hasNext())
    {
        QString filePath = it.next();
        addMediaFile(filePath);
        logger.info("Add media file: %s", filePath.toStdString().data());
    }
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
        logger.error("video path get failed.");
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
    m_sliderTimer->start(200);

    return true;
}

void ButtomBar::slotSliderPressed()
{
    m_sliderTimer->stop();
}

void ButtomBar::slotSliderReleased()
{
    if(m_playController->getMediaPlayInfo()->mediaName == "")
    {
        logger.warning("No media is currently playing.");
        return;
    }

    logger.info("slider value: %d", m_timeSlider->value());
    //修改当前时长
    int currentTime = m_timeSlider->value();
    QString currentTimeStr = QString::fromStdString(m_playController->timeFormatting(currentTime));
    m_currentTime->setText(currentTimeStr);

    //修改进度
    m_playController->changePlayProgress(currentTime);
    m_sliderTimer->start(200);
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
        }
        else
        {
            //继续
            m_playController->continuePlay();

            QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPause);
            m_playBtn->setIcon(playIcon);
        }
    }
}

void ButtomBar::slotAddMediaFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "选择媒体文件", "./", "*.mp4 *.wav *.mp3");
    if(filePath == "")
        return;

    addMediaFile(filePath);
}

void ButtomBar::addMediaFile(QString filePath)
{
    if(QFile::exists(filePath) == false)
    {
        logger.warning("the file is not exist: %s", filePath.toStdString().data());
        return;
    }

    int duration = m_playController->getMediaDuration(filePath.toStdString());
    QString timetotalStr = QString::fromStdString(m_playController->timeFormatting(duration));

    // 创建缩略图，音频文件无需创建缩略图
    QString fileExtension = filePath.section('.', -1); // 取最后一个点后面的部分
    if(fileExtension != "mp3" && fileExtension != "wav")
    {
        QString thumbnailPath = filePath;

        if(filePath.contains("."))
            thumbnailPath = thumbnailPath.replace(QRegExp("\\.[^.]+$"), ".bmp");
        else
            thumbnailPath += ".bmp";

        if(QFile::exists(thumbnailPath) == false)
            m_playController->saveFrameToBmp(filePath.toStdString().data(), thumbnailPath.toStdString().data(), 5);     //创建缩略图

        m_playList->addMediaItem(thumbnailPath, filePath, timetotalStr, "未观看");
    }
    else
        m_playList->addMediaItem("", filePath, timetotalStr, "未观看");
}

void ButtomBar::slotChangeSpeed()
{
    // 获取下一个播放速率索引
    speedIndex = (speedIndex + 1) % m_playbackSpeeds.size();

    // 获取播放速率
    float speedFactor = m_playbackSpeeds[speedIndex];
    m_changeSpeedBtn->setText(QString::number(speedFactor));
    if(speedFactor == 1.0)
        m_changeSpeedBtn->setText("倍速");

    m_playController->changePlaySpeed(speedFactor);
}

void ButtomBar::slotVolumeChanged()
{
    m_playController->changeVolume(m_volumeSlider->value());
}

void ButtomBar::slotUpdateProgress()
{
    logger.debug("progress: %f", m_playController->getPlayProgress());
    int currentPlayProgress = static_cast<int>(m_playController->getPlayProgress() + 0.2);  //增加0.2s冗余时间，缓解进度条走不满的问题

    //进度条更新
    if (currentPlayProgress <= m_timeSlider->maximum())
    {
        m_timeSlider->setValue(currentPlayProgress);
    }

    //当前时长增加
    QString currentTimeStr = QString::fromStdString(m_playController->timeFormatting(currentPlayProgress));
    m_currentTime->setText(currentTimeStr);

    //播放完成
    if(currentPlayProgress == m_timeSlider->maximum())
    {
        m_playController->endPlay();
        m_sliderTimer->stop();
        QIcon playIcon = QApplication::style()->standardIcon(QStyle::SP_MediaPlay);
        m_playBtn->setIcon(playIcon);
    }
}
