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

class PlayList : public QListWidget
{
    Q_OBJECT
public:
    explicit PlayList(QWidget *parent = nullptr);

    void setPlayController(PlayController* playController) {m_playController = playController;};
    QString getMediaPath();
    void addMediaFile(QString filePath);
    void searchMediaFiles(const QString &directoryPath);

private:
    PlayController* m_playController;
};

#endif // PLAYLIST_H
