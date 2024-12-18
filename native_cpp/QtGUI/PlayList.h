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
#include <QDir>
#include <QDirIterator>
#include "PlayController.h"


class PlayList : public QListWidget
{
    Q_OBJECT
public:
    explicit PlayList(QWidget *parent = nullptr);

    void setPlayController(PlayController* playController) {m_playController = playController;}
    void setMediaDirPath(QString mediaDirPath);
    QString getMediaPath();
    void searchMediaFiles();

signals:
    void sigPlayMedia(QString mediaPath, bool cameraInput = false);

public slots:
    void slotAddMediaItem(QString filePath);

private:
    PlayController* m_playController;

    QString m_mediaDirPath;
};

#endif // PLAYLIST_H
