#include "PlayList.h"
#include "Logger.h"
#include "BmpAndWavAchieve.h"

PlayList::PlayList(QWidget *parent) : QListWidget(parent)
{
}

QString PlayList::getVideoPath()
{
    // 获取当前选中的 QListWidgetItem
    QListWidgetItem *currentItem = this->currentItem();
    if(!currentItem)
    {
        logger.error("No item selected.");
        return "";
    }

    // 获取对应的 QWidget
    QWidget *itemWidget = this->itemWidget(currentItem);
    if (!itemWidget)
    {
        logger.error("Item widget not found.");
        return "";
    }

    // 查找所有 QLabel
    QList<QLabel *> labels = itemWidget->findChildren<QLabel *>();
    if (labels.size() < 2)  // 确保有至少两个 QLabel
    {
        logger.error("Less than two QLabel found.");
        return "";
    }

    // 获取标题文本，在第二个label
    QString videoPath = labels[1]->text();
    return videoPath;
}


void PlayList::addVideoItem(const QString &thumbnailPath, const QString &title, const QString &duration, const QString &status)
{
    if(QFile::exists(title) == false)
    {
        logger.error("media file is not exist");
        return;
    }

    //创建item
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 100)); // 设置每个项的大小

    QWidget *itemWidget = new QWidget(this);
    QHBoxLayout *hBox = new QHBoxLayout;
    QHBoxLayout *hBox2 = new QHBoxLayout;
    QVBoxLayout *vBox = new QVBoxLayout;


    // 添加封面图像，无封面则填充文件名
    QLabel *thumbnail = new QLabel();
    thumbnail->setFixedSize(80, 60);
    if(thumbnailPath != "" && QFile::exists(thumbnailPath) == true)
    {
        QPixmap pixmap(thumbnailPath);
        thumbnail->setPixmap(pixmap.scaled(80, 60, Qt::KeepAspectRatio)); // 调整封面图像大小
    }
    else
    {
        QFileInfo fileInfo(title);
        QString fileName = fileInfo.fileName();
        thumbnail->setAlignment(Qt::AlignCenter);
        thumbnail->setText(fileName);
    }


    // 添加标题和其他信息
    QLabel *titleLabel = new QLabel(title);
    QLabel *durationLabel = new QLabel(duration);
    QLabel *statusLabel = new QLabel(status);

    // 设置布局
    hBox->addWidget(durationLabel);
    hBox->addWidget(statusLabel);
    vBox->addSpacing(25);
    vBox->addWidget(titleLabel);
    vBox->addLayout(hBox);
    vBox->addSpacing(25);
    hBox2->addWidget(thumbnail);
    hBox2->addLayout(vBox);
    itemWidget->setLayout(hBox2);

    this->addItem(item);
    this->setItemWidget(item, itemWidget);
}
