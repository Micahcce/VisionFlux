#include "PlayList.h"
#include "Logger.h"

PlayList::PlayList(QWidget *parent) : QListWidget(parent)
{
}

void PlayList::setMediaDirPath(QString mediaDirPath)
{
    m_mediaDirPath = mediaDirPath;
}

QString PlayList::getMediaPath()
{
    // 获取当前选中的 QListWidgetItem
    QListWidgetItem *currentItem = this->currentItem();
    if(!currentItem)
    {
        logger.error("No item selected.");
        return "";
    }

    // 从 item 中获取存储的文件路径
    QString mediaPath = currentItem->data(Qt::UserRole).toString();

    if (mediaPath.isEmpty())
    {
        logger.error("Media path is empty.");
        return "";
    }

    return mediaPath;
}

void PlayList::slotAddMediaItem(QString filePath)
{
    if(QFile::exists(filePath) == false)
    {
        logger.warning("the file is not exist: %s", filePath.toStdString().data());
        return;
    }

    // 添加缩略图作为封面，音频文件则显示文件名
    QLabel *thumbnail = new QLabel();
    thumbnail->setFixedSize(80, 60);

    QString thumbnailPath = filePath;
    QString fileExtension = filePath.section('.', -1); // 获取文件扩展名
    if(fileExtension != "mp3" && fileExtension != "wav")
    {
        if(filePath.contains("."))
            thumbnailPath = thumbnailPath.replace(QRegExp("\\.[^.]+$"), ".bmp");
        else
            thumbnailPath += ".bmp";

        // 缩略图不存在则创建
        if(QFile::exists(thumbnailPath) == false)
            m_playController->saveFrameToBmp(filePath.toStdString().data(), thumbnailPath.toStdString().data(), 5);

        QPixmap pixmap(thumbnailPath);
        thumbnail->setPixmap(pixmap.scaled(80, 60, Qt::KeepAspectRatio)); // 调整封面图像大小
    }
    else
    {
        thumbnail->setAlignment(Qt::AlignCenter);
        thumbnail->setText(fileExtension);
    }

    // 创建控件
    QWidget *itemWidget = new QWidget(this);

    // 获取时长
    int duration = m_playController->getMediaDuration(filePath.toStdString());
    QString timetotalStr = QString::fromStdString(m_playController->timeFormatting(duration));

    // 添加标题、时长和状态
    QFileInfo fileInfo(filePath);
    QLabel *titleLabel = new QLabel(fileInfo.fileName());
    QLabel *durationLabel = new QLabel(timetotalStr);
    QLabel *statusLabel = new QLabel("未观看");

    // 设置布局
    QHBoxLayout *hBox = new QHBoxLayout;
    QHBoxLayout *hBox2 = new QHBoxLayout;
    QVBoxLayout *vBox = new QVBoxLayout;

    hBox->addWidget(durationLabel);
    hBox->addWidget(statusLabel);
    vBox->addSpacing(25);
    vBox->addWidget(titleLabel);
    vBox->addLayout(hBox);
    vBox->addSpacing(25);
    hBox2->addWidget(thumbnail);
    hBox2->addLayout(vBox);
    itemWidget->setLayout(hBox2);

    //创建item
    QListWidgetItem *item = new QListWidgetItem();
    item->setData(Qt::UserRole, filePath);                 // 将文件路径存储在 item 的 Qt::UserRole 中
    item->setSizeHint(QSize(0, 100));                      // 设置每个项的大小
    this->addItem(item);
    this->setItemWidget(item, itemWidget);
}

void PlayList::searchMediaFiles()
{
    // 创建 QDirIterator 以递归方式搜索文件
    QStringList allowedExtensions = QString::fromStdString(m_playController->getAllowedExtensions()).split(' ');
    QDirIterator it(m_mediaDirPath, allowedExtensions, QDir::Files, QDirIterator::Subdirectories);

    // 迭代找到的文件
    while (it.hasNext())
    {
        QString filePath = it.next();
        slotAddMediaItem(filePath);
        logger.info("Add media file: %s", filePath.toStdString().data());
    }
}

