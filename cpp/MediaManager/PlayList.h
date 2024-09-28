#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <iostream>
#include <QListWidget>
#include <QString>
#include <QImage>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

class PlayList : public QListWidget
{
    Q_OBJECT
public:
    explicit PlayList(QWidget *parent = nullptr);

    QString getVideoPath();

private:
    void extractThumbnail(const char* videoFilePath, const char* outputImagePath);
    void addVideoItem(const QString &title, const QString &duration, const QString &status, const QString &thumbnailPath);
};

#endif // PLAYLIST_H
