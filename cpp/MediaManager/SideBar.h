#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <iostream>
#include <QTabWidget>
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QImage>
#include <QDebug>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

class SideBar : public QTabWidget
{
    Q_OBJECT
public:
    SideBar(QWidget *parent = nullptr);

    QString getVideoPath();

public slots:
    void slotVideoDoubleClicked();

private:
    void extractThumbnail(const char* videoFilePath, const char* outputImagePath);
    void addVideoItem(const QString &title, const QString &duration, const QString &status, const QString &thumbnailPath);

    QWidget* m_playListTab;
    QWidget* m_videoSummaryTab;
    QListWidget* m_playList;

};

#endif // SIDEBAR_H
