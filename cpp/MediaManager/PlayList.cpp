#include "PlayList.h"
#include "Logger.h"

PlayList::PlayList(QWidget *parent) : QListWidget(parent)
{
    //Debug
    {
        // 添加视频项
        addVideoItem("C:\\Users\\13055\\Desktop\\output.mp4", "00:07", "未观看");
        addVideoItem("C:\\Users\\13055\\Desktop\\aki.mp4", "00:10", "未观看");

    }
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

//提取封面
void PlayList::extractThumbnail(const char* videoFilePath, const char* outputImagePath) {
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoFilePath, nullptr, nullptr) < 0) {
        logger.error("Could not open video file: %s", videoFilePath);
        return;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        logger.error("Could not find stream info.");
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
        logger.error("Could not find a video stream.");
        avformat_close_input(&formatCtx);
        return;
    }

    AVCodecParameters* codecpar = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        logger.error("Could not find decoder for codec ID: %d", codecpar->codec_id);
        avformat_close_input(&formatCtx);
        return;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        logger.error("Could not allocate codec context.");
        avformat_close_input(&formatCtx);
        return;
    }

    if (avcodec_parameters_to_context(codecCtx, codecpar) < 0) {
        logger.error("Could not copy codec parameters to codec context.");
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        logger.error("Could not open codec.");
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* frameRGB = av_frame_alloc();

    if (!frameRGB || !packet || !frame) {
        logger.error("Could not allocate frame or packet.");
        av_frame_free(&frameRGB);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    // 设置输出图像的参数
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, codecCtx->width, codecCtx->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB32, codecCtx->width, codecCtx->height, 1);

    // 提取第1、2、3秒的帧
    bool succeed = false;

    // 初始化swsCtx（假设codecCtx的参数在整个过程中保持不变）
    SwsContext* swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB32,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsCtx)
    {
        logger.error("Failed to initialize sws context.");
        av_freep(&buffer);
        av_frame_free(&frameRGB);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    for (int i = 0; i < 3 && !succeed; i++)
    {  // 成功保存后结束循环
        int64_t timestamp = av_rescale_q(i * AV_TIME_BASE, AV_TIME_BASE_Q, formatCtx->streams[videoStreamIndex]->time_base);
        if (av_seek_frame(formatCtx, videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0)
        {
            logger.error("Could not seek to frame at %d second.", i);
            continue;
        }

        while (av_read_frame(formatCtx, packet) >= 0)
        {
            if (packet->stream_index == videoStreamIndex)
            {
                if (avcodec_send_packet(codecCtx, packet) >= 0)
                {
                    int receiveResult = avcodec_receive_frame(codecCtx, frame);
                    if (receiveResult == AVERROR(EAGAIN))
                        continue;
                    else if (receiveResult < 0)
                        break;

                    // 使用已经初始化的swsCtx进行转换
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, frameRGB->data, frameRGB->linesize);

                    // 使用QImage保存图像
                    QImage image((const uchar*)buffer, codecCtx->width, codecCtx->height, QImage::Format_RGB32);
                    if (image.save(outputImagePath))
                    {
                        succeed = true;
                        logger.info("Image saved successfully at: %s", outputImagePath);
                        break;  // 成功后退出 while 循环
                    }
                }
            }
            av_packet_unref(packet);  // 释放数据包
        }
    }

    // 释放swsCtx
    sws_freeContext(swsCtx);

    if (!succeed)
    {
        logger.debug("Failed to save any image.");
    }

    // 清理资源
    av_freep(&buffer);
    av_frame_free(&frameRGB);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
}



void PlayList::addVideoItem(const QString &title, const QString &duration, const QString &status)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(0, 100)); // 设置每个项的大小

    QWidget *itemWidget = new QWidget();
    QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);

    // 缩略图名称
    QString outputImagePath = title;
    if(outputImagePath.contains("."))
        outputImagePath = outputImagePath.replace(QRegExp("\\.[^.]+$"), ".png");
    else
        outputImagePath += ".png";

    // 不存在则创建
    if(QFile::exists(outputImagePath) == false)
        extractThumbnail(title.toStdString().data(), outputImagePath.toStdString().data());

    // 添加封面图像
    QLabel *thumbnail = new QLabel();
    QPixmap pixmap(outputImagePath);
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

    this->addItem(item);
    this->setItemWidget(item, itemWidget);
}
