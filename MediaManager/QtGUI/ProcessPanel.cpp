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

    m_streamUrlEdit = new QLineEdit(this);
    m_streamUrlEdit->setFont(font);

    QPushButton* StreamUrlPlayBtn = new QPushButton(this);
    StreamUrlPlayBtn->setText("播放");
    connect(StreamUrlPlayBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamPlay);

    QPushButton* StreamUrlSaveBtn = new QPushButton(this);
    StreamUrlSaveBtn->setText("保存");
    connect(StreamUrlSaveBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamSave);

    QPushButton* StreamUrlEndBtn = new QPushButton(this);
    StreamUrlEndBtn->setText("结束");
    connect(StreamUrlEndBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamEnd);

    //转码保存或推流
    QLabel* LocalFileLabel = new QLabel(this);
    LocalFileLabel->setText("本地文件");
    LocalFileLabel->setFont(font);

    m_localFileEdit = new QLineEdit(this);
    m_localFileEdit->setFont(font);

    QPushButton* localFileSelectBtn = new QPushButton(this);
    localFileSelectBtn->setText("选择文件");

    QLabel* TargetFormatLabel = new QLabel(this);
    TargetFormatLabel->setText("目标格式");
    TargetFormatLabel->setFont(font);

    QComboBox* TargetFormatCb = new QComboBox(this);
    TargetFormatCb->addItem("mp3");
    TargetFormatCb->addItem("wav");
    TargetFormatCb->addItem("mp4");
    TargetFormatCb->addItem("flv");

    QPushButton* localFileConverBtn = new QPushButton(this);
    localFileConverBtn->setText("转换");

    QLabel* ServerAddressLabel = new QLabel(this);
    ServerAddressLabel->setText("服务器地址");
    ServerAddressLabel->setFont(font);

    m_serverAddressEdit = new QLineEdit(this);
    m_serverAddressEdit->setFont(font);

    QPushButton* PullStreamBtn = new QPushButton(this);
    PullStreamBtn->setText("推流");


    // 创建一个容器，用于放置控件
    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(container);

    layout->addWidget(StreamUrlLabel);
    layout->addWidget(m_streamUrlEdit);
    layout->addWidget(StreamUrlPlayBtn);
    layout->addWidget(StreamUrlSaveBtn);
    layout->addWidget(StreamUrlEndBtn);
    layout->addWidget(LocalFileLabel);
    layout->addWidget(m_localFileEdit);
    layout->addWidget(localFileSelectBtn);
    layout->addWidget(TargetFormatLabel);
    layout->addWidget(TargetFormatCb);
    layout->addWidget(localFileConverBtn);
    layout->addWidget(ServerAddressLabel);
    layout->addWidget(m_serverAddressEdit);
    layout->addWidget(PullStreamBtn);

    // 将容器设置为 QScrollArea 的子组件
    container->setLayout(layout);
    setWidget(container);
}

void ProcessPanel::slotLiveStreamPlay()
{
    QString streamUrl = m_streamUrlEdit->text();
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
