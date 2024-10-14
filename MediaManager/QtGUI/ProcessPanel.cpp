#include "ProcessPanel.h"

ProcessPanel::ProcessPanel(QWidget *parent) : QWidget(parent)
{
    QFont font("微软雅黑");

    //网络流播放与保存
    QLabel* StreamUrlLabel = new QLabel(this);
    StreamUrlLabel->setGeometry(10, 10, 60, 40);
    StreamUrlLabel->setText("网络流");
    StreamUrlLabel->setFont(font);

    m_streamUrlEdit = new QLineEdit(this);
    m_streamUrlEdit->setGeometry(10, 60, 270, 40);
    m_streamUrlEdit->setFont(font);

    QPushButton* StreamUrlPlayBtn = new QPushButton(this);
    StreamUrlPlayBtn->setGeometry(10, 110, 100, 40);
    StreamUrlPlayBtn->setText("播放");
    connect(StreamUrlPlayBtn, &QPushButton::clicked, this, &ProcessPanel::slotLiveStreamPlay);

    QPushButton* StreamUrlSaveBtn = new QPushButton(this);
    StreamUrlSaveBtn->setGeometry(10, 160, 100, 40);
    StreamUrlSaveBtn->setText("保存");
}

void ProcessPanel::slotLiveStreamPlay()
{
//    QString streamUrl = m_streamUrlEdit->text();
    m_bottomBar->slotStartPlayMedia();
}
