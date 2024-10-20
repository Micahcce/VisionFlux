#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <iostream>
#include <QListWidget>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QFile>
#include <QPainter>
#include <QDir>
#include <QDirIterator>
#include "PlayController.h"
#include "BottomBar.h"

class PlayList : public QListWidget
{
    Q_OBJECT
public:
    explicit PlayList(QWidget *parent = nullptr);

    void setBottomBar(BottomBar* bottomBar);
    void setMediaDirPath(QString mediaDirPath);
    QString getMediaPath();
    void searchMediaFiles();

public slots:
    void slotAddMediaItem(QString filePath);

private slots:
    void slotStartPlayMedia();
    void slotSetSelectedMediaPath();

private:
    BottomBar* m_bottomBar;

    QStringList m_allowedExtensions;
    QString m_mediaDirPath;
};

#endif // PLAYLIST_H
