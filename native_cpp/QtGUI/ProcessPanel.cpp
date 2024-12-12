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
    connect(StreamUrlPlayBtn, &QPushButton::clicked, this, [this]{emit sigLiveStreamPlay(m_pullStreamUrlEdit->text());});

    QPushButton* StreamUrlSaveBtn = new QPushButton(this);
    StreamUrlSaveBtn->setText("保存");
    connect(StreamUrlSaveBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamSave);

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

    m_targetFormatCb = new QComboBox(this);
    m_targetFormatCb->addItem("mp3");
    m_targetFormatCb->addItem("wav");
    m_targetFormatCb->addItem("mp4");
    m_targetFormatCb->addItem("flv");

    QPushButton* LocalFileConverBtn = new QPushButton(this);
    LocalFileConverBtn->setText("转换");
    connect(LocalFileConverBtn, &QPushButton::clicked, this, &ProcessPanel::slotConvert);

    QLabel* AllEndLabel = new QLabel(this);
    AllEndLabel->setText("全部结束");
    AllEndLabel->setFont(font);

    QPushButton* AllEndBtn = new QPushButton(this);
    AllEndBtn->setText("结束");
    connect(AllEndBtn, &QPushButton::clicked, this, &ProcessPanel::slotAllEnd);


    // 创建一个容器，用于放置控件
    QWidget *container = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(container);

    layout->addWidget(StreamUrlLabel);
    layout->addWidget(m_pullStreamUrlEdit);
    layout->addWidget(StreamUrlPlayBtn);
    layout->addWidget(StreamUrlSaveBtn);
    layout->addWidget(PushStreamLabel);
    layout->addWidget(m_pushStreamUrlEdit);
    layout->addWidget(m_pushStreamFileEdit);
    layout->addWidget(PushStreamFileSelectBtn);
    layout->addWidget(PushStreamBtn);
    layout->addWidget(ConvertLabel);
    layout->addWidget(m_convertFileEdit);
    layout->addWidget(ConvertFileSelectBtn);
    layout->addWidget(m_targetFormatCb);
    layout->addWidget(LocalFileConverBtn);
    layout->addWidget(AllEndLabel);
    layout->addWidget(AllEndBtn);


    // 将容器设置为 QScrollArea 的子组件
    container->setLayout(layout);
    setWidget(container);
}

void ProcessPanel::slotLiveStreamSave()
{
    const QString streamUrl = m_pullStreamUrlEdit->text();
    QUrl qUrl(streamUrl);                                  // 将 URL 转换为 QUrl 对象
    QString fileName = QFileInfo(qUrl.path()).fileName();  // 获取文件名
    QString outputPath = QFileInfo(fileName).baseName();   // 获取不带后缀的文件名
    outputPath += ".flv";                                  // flv格式

    m_playController->streamConvert(streamUrl.toStdString(), outputPath.toStdString());
}

void ProcessPanel::slotPushStreamFileSelect()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "选择媒体文件", "../media/", QString::fromStdString(m_playController->getAllowedExtensions()));
    if(filePath == "")
        return;

    m_pushStreamFileEdit->setText(filePath);
}

void ProcessPanel::slotPushStream()
{
    QString streamUrl = m_pushStreamUrlEdit->text();
    QString filePath = m_pushStreamFileEdit->text();
    m_playController->streamConvert(filePath.toStdString(), streamUrl.toStdString());
}

void ProcessPanel::slotConvertFileSelect()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "选择媒体文件", "../media/", QString::fromStdString(m_playController->getAllowedExtensions()));
    if(filePath == "")
        return;

    m_convertFileEdit->setText(filePath);
}

void ProcessPanel::slotConvert()
{
    QString filePath = m_convertFileEdit->text();
    QString dstFormat = filePath;
    if(dstFormat.contains("."))
        dstFormat = dstFormat.replace(QRegExp("\\.[^.]+$"), "." + m_targetFormatCb->currentText());
    else
        dstFormat = dstFormat + "." + m_targetFormatCb->currentText();
    m_playController->streamConvert(filePath.toStdString(), dstFormat.toStdString());
}

void ProcessPanel::slotAllEnd()
{
    logger.info("To be developed");
}
