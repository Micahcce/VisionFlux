#include "ProcessPanel.h"

ProcessPanel::ProcessPanel(QWidget *parent) : QScrollArea(parent)
{
    setWidgetResizable(true);

    //字体
    QFont font("微软雅黑");

    //网络流播放与保存
    QLabel* StreamUrlLabel = new QLabel(this);
    StreamUrlLabel->setText("网络流");
    StreamUrlLabel->setFont(font);

    m_pullStreamUrlEdit = new QLineEdit(this);
    m_pullStreamUrlEdit->setPlaceholderText("拉流地址");
    m_pullStreamUrlEdit->setFont(font);

    QPushButton* StreamUrlPlayBtn = new QPushButton(this);
    StreamUrlPlayBtn->setText("播放");
    connect(StreamUrlPlayBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamPlay);

    QPushButton* StreamUrlSaveBtn = new QPushButton(this);
    StreamUrlSaveBtn->setText("保存");
    connect(StreamUrlSaveBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamSave);

    QPushButton* StreamUrlEndBtn = new QPushButton(this);
    StreamUrlEndBtn->setText("结束");
    connect(StreamUrlEndBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamEnd);

    //推流
    QLabel* PushStreamLabel = new QLabel(this);
    PushStreamLabel->setText("推流");
    PushStreamLabel->setFont(font);

    m_pushStreamUrlEdit = new QLineEdit(this);
    m_pushStreamUrlEdit->setPlaceholderText("推流地址");
    m_pushStreamUrlEdit->setFont(font);

    m_pushStreamFileEdit = new QLineEdit(this);
    m_pushStreamFileEdit->setPlaceholderText("文件地址");
    m_pushStreamFileEdit->setFont(font);

    QPushButton* PushStreamFileSelectBtn = new QPushButton(this);
    PushStreamFileSelectBtn->setText("选择文件");
    connect(PushStreamFileSelectBtn, &QPushButton::clicked, this, &ProcessPanel::slotPushStreamFileSelect);

    QPushButton* PushStreamBtn = new QPushButton(this);
    PushStreamBtn->setText("推流");
    connect(PushStreamBtn, &QPushButton::clicked, this, &ProcessPanel::slotPushStream);

    //格式转换
    m_convertFileEdit = new QLineEdit(this);
    m_convertFileEdit->setPlaceholderText("文件地址");
    m_convertFileEdit->setFont(font);

    QPushButton* ConvertFileSelectBtn = new QPushButton(this);
    ConvertFileSelectBtn->setText("选择文件");
    connect(ConvertFileSelectBtn, &QPushButton::clicked, this, &ProcessPanel::slotConvertFileSelect);

    QLabel* ConvertLabel = new QLabel(this);
    ConvertLabel->setText("格式转换");
    ConvertLabel->setFont(font);

    QComboBox* TargetFormatCb = new QComboBox(this);
    TargetFormatCb->addItem("目标格式: mp3");
    TargetFormatCb->addItem("目标格式: wav");
    TargetFormatCb->addItem("目标格式: mp4");
    TargetFormatCb->addItem("目标格式: flv");

    QPushButton* LocalFileConverBtn = new QPushButton(this);
    LocalFileConverBtn->setText("转换");


    // 创建一个容器，用于放置控件
    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(container);

    layout->addWidget(StreamUrlLabel);
    layout->addWidget(m_pullStreamUrlEdit);
    layout->addWidget(StreamUrlPlayBtn);
    layout->addWidget(StreamUrlSaveBtn);
    layout->addWidget(StreamUrlEndBtn);
    layout->addWidget(PushStreamLabel);
    layout->addWidget(m_pushStreamUrlEdit);
    layout->addWidget(m_pushStreamFileEdit);
    layout->addWidget(PushStreamFileSelectBtn);
    layout->addWidget(PushStreamBtn);
    layout->addWidget(ConvertLabel);
    layout->addWidget(m_convertFileEdit);
    layout->addWidget(ConvertFileSelectBtn);
    layout->addWidget(TargetFormatCb);
    layout->addWidget(LocalFileConverBtn);


    // 将容器设置为 QScrollArea 的子组件
    container->setLayout(layout);
    setWidget(container);
}

void ProcessPanel::slotLiveStreamPlay()
{
    QString streamUrl = m_pullStreamUrlEdit->text();
    m_bottomBar->startPlayMedia(streamUrl);
}

void ProcessPanel::slotLiveStreamSave()
{
    logger.info("To be developed");
}

void ProcessPanel::slotLiveStreamEnd()
{
    logger.info("To be developed");
}

void ProcessPanel::slotPushStreamFileSelect()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "选择媒体文件", "../media/", "*.mp4 *.wav *.mp3");
    if(filePath == "")
        return;

    m_pushStreamFileEdit->setText(filePath);
}

void ProcessPanel::slotPushStream()
{
    QString streamUrl = m_pushStreamUrlEdit->text();
    QString filePath = m_pushStreamFileEdit->text();
    m_bottomBar->getPlayController()->pushStream(filePath.toStdString(), streamUrl.toStdString());
//    m_bottomBar->getPlayController()->pushStream("C:\\Users\\13055\\Desktop\\VisionFlux\\media\\output.mp4",
    //                                                 "rtmp://localhost:1935/live/livestream");
}

void ProcessPanel::slotConvertFileSelect()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "选择媒体文件", "../media/", "*.mp4 *.wav *.mp3");
    if(filePath == "")
        return;

    m_convertFileEdit->setText(filePath);
}
