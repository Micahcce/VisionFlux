#include "SideBar.h"

SideBar::SideBar(QWidget *parent) : QTabWidget(parent)
{
    setStyleSheet("background-color:#EEEEDD;");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(280);

    QFont font("微软雅黑");
    m_playListTab = new QWidget(this);
    m_videoSummaryTab = new QWidget(this);
    m_playListTab->resize(100, 40);
    m_videoSummaryTab->resize(100, 40);
    setFont(font);
    addTab(m_playListTab, "播放列表");
    addTab(m_videoSummaryTab, "AI总结");
    setStyleSheet("QTabBar::tab { width: 100px; height: 40px;background-color:transparent;color:rgb(120,120,120);}"
          "QTabBar::tab:selected{ color:rgb(75,75,110); border-bottom:2px solid#4b4b6e; }"
          "QTabBar::tab:hover{ color:rgb(0,0,0); }"
                  "QTabWidget:pane{border:0px; background-color:transparent}");


    //播放列表
    m_playList = new QListWidget(m_playListTab);
    m_playList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_playList, &QListWidget::itemDoubleClicked, this, &SideBar::slotVideoDoubleClicked);

    QVBoxLayout* vBox = new QVBoxLayout;
    vBox->addWidget(m_playList);
    m_playListTab->setLayout(vBox);

    //提取封面
    extractThumbnail("C:\\Users\\13055\\Desktop\\aki.mp4", "C:\\Users\\13055\\Desktop\\aki.png");
    extractThumbnail("C:\\Users\\13055\\Desktop\\output.mp4", "C:\\Users\\13055\\Desktop\\output.png");

    // 添加视频项
    addVideoItem("C:\\Users\\13055\\Desktop\\aki.mp4", "00:10", "未观看", "C:\\Users\\13055\\Desktop\\aki.mp4");
    addVideoItem("C:\\Users\\13055\\Desktop\\output.mp4", "00:07", "未观看", "C:\\Users\\13055\\Desktop\\output.png");
}

QString SideBar::getVideoPath()
{
    // 获取当前选中的 QListWidgetItem
    QListWidgetItem *currentItem = m_playList->currentItem();
    if(!currentItem)
    {
        std::cerr << "No item selected." << std::endl;;
        return nullptr;
    }

    // 获取对应的 QWidget
    QWidget *itemWidget = m_playList->itemWidget(currentItem);
    if (!itemWidget)
    {
        std::cerr << "Item widget not found." << std::endl;;
        return nullptr;
    }

    // 查找所有 QLabel
    QList<QLabel *> labels = itemWidget->findChildren<QLabel *>();
    if (labels.size() < 2)  // 确保有至少两个 QLabel
    {
        std::cerr << "Less than two QLabel found." << std::endl;;
        return nullptr;
    }

    // 获取标题文本，在第二个label
    QString videoPath = labels[1]->text();
    return videoPath;
}

void SideBar::slotVideoDoubleClicked()
{

}


//提取封面
void SideBar::extractThumbnail(const char* videoFilePath, const char* outputImagePath) {
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoFilePath, nullptr, nullptr) < 0) {
        std::cerr << "Could not open video file: " << videoFilePath << std::endl;
        return;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "Could not find stream info." << std::endl;
        avformat_close_input(&formatCtx);
        return;
    }

    int videoStreamIndex = -1;
    for (int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "Could not find a video stream." << std::endl;
        avformat_close_input(&formatCtx);
        return;
    }

    AVCodecParameters* codecpar = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Could not find decoder for codec ID: " << codecpar->codec_id << std::endl;
        avformat_close_input(&formatCtx);
        return;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        std::cerr << "Could not allocate codec context." << std::endl;
        avformat_close_input(&formatCtx);
        return;
    }

    if (avcodec_parameters_to_context(codecCtx, codecpar) < 0) {
        std::cerr << "Could not copy codec parameters to codec context." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* frameRGB = av_frame_alloc();

    if (!frameRGB || !packet || !frame) {
        std::cerr << "Could not allocate frame or packet." << std::endl;
        av_frame_free(&frameRGB);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    // 设置输出图像的参数
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecCtx->width, codecCtx->height, 32);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB24, codecCtx->width, codecCtx->height, 32);

    // 提取第1秒的帧
    if (av_seek_frame(formatCtx, videoStreamIndex, 1 * AV_TIME_BASE, AVSEEK_FLAG_ANY) >= 0) {
        while (av_read_frame(formatCtx, packet) >= 0) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecCtx, packet) >= 0) {
                    if (avcodec_receive_frame(codecCtx, frame) >= 0) {
                        // 转换为 RGB
                        SwsContext* swsCtx = sws_getContext(
                            codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                            codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24,
                            SWS_BILINEAR, nullptr, nullptr, nullptr
                        );
                        if (swsCtx) {
                            sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, frameRGB->data, frameRGB->linesize);
                            sws_freeContext(swsCtx);

                            // 使用 QImage 保存图像
                            QImage image((const uchar*)buffer, codecCtx->width, codecCtx->height, QImage::Format_RGB888);
                            if (!image.save(outputImagePath)) {
                                std::cerr << "Failed to save image at " << outputImagePath << std::endl;
                            } else {
                                std::cout << "Image saved successfully at " << outputImagePath << std::endl;
                            }
                        } else {
                            std::cerr << "Failed to initialize sws context." << std::endl;
                        }
                        break; // 找到帧后退出循环
                    }
                }
            }
            av_packet_unref(packet); // 释放数据包
        }
    } else {
        std::cerr << "Could not seek to frame." << std::endl;
    }

    // 清理资源
    av_freep(&buffer);
    av_frame_free(&frameRGB);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
}


void SideBar::addVideoItem(const QString &title, const QString &duration, const QString &status, const QString &thumbnailPath)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 100)); // 设置每个项的大小

    QWidget *itemWidget = new QWidget();
    QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);

    // 添加封面图像
    QLabel *thumbnail = new QLabel();
    QPixmap pixmap(thumbnailPath);
    thumbnail->setPixmap(pixmap.scaled(80, 60, Qt::KeepAspectRatio)); // 调整封面图像大小

    // 添加标题和其他信息
    QLabel *titleLabel = new QLabel(title);
    QLabel *durationLabel = new QLabel(duration);
    QLabel *statusLabel = new QLabel(status);

    // 设置布局
    itemLayout->addWidget(thumbnail);
    itemLayout->addWidget(titleLabel);
    itemLayout->addWidget(durationLabel);
    itemLayout->addWidget(statusLabel);
    itemWidget->setLayout(itemLayout);

    m_playList->addItem(item);
    m_playList->setItemWidget(item, itemWidget);
}
