#include "PlayList.h"
#include "Logger.h"
#include "BmpAndWavAchieve.h"

PlayList::PlayList(QWidget *parent) : QListWidget(parent)
{
}

void PlayList::setBottomBar(BottomBar *bottomBar)
{
    m_bottomBar = bottomBar;
    connect(this, &QListWidget::itemDoubleClicked, this, &PlayList::slotStartPlayMedia);    //双击播放
    connect(this, &QListWidget::itemClicked, this, &PlayList::slotSetSelectedMediaPath);    //设置选中项的文件地址
    connect(m_bottomBar, &BottomBar::sigAddMediaItem, this, &PlayList::slotSetSelectedMediaPath);   //添加播放项
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
    QString mediaPath = labels[1]->text();
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
    QString fileExtension = filePath.section('.', -1); // 取最后一个点后面的部分
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
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        thumbnail->setAlignment(Qt::AlignCenter);
        thumbnail->setText(fileName);
    }

    //获取时长
    int duration = m_playController->getMediaDuration(filePath.toStdString());
    QString timetotalStr = QString::fromStdString(m_playController->timeFormatting(duration));

    // 添加标题、时长和状态
    QLabel *titleLabel = new QLabel(filePath);
    QLabel *durationLabel = new QLabel(timetotalStr);
    QLabel *statusLabel = new QLabel("未观看");


    //创建item
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 100)); // 设置每个项的大小

    // 设置布局
    QWidget *itemWidget = new QWidget(this);
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

    this->addItem(item);
    this->setItemWidget(item, itemWidget);
}

void PlayList::searchMediaFiles(const QString &directoryPath)
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
        slotAddMediaItem(filePath);
        logger.info("Add media file: %s", filePath.toStdString().data());
    }
}

void PlayList::slotStartPlayMedia()
{
    QString mediaPath = getMediaPath();
    m_bottomBar->startPlayMedia(mediaPath);
}

void PlayList::slotSetSelectedMediaPath()
{
    QString mediaPath = getMediaPath();
    m_bottomBar->setSelectMediaPath(mediaPath);
}
