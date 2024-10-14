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
    QString getMediaPath();
    void searchMediaFiles(const QString &directoryPath);

public slots:
    void slotAddMediaItem(QString filePath);

private slots:
    void slotStartPlayMedia();
    void slotSetSelectedMediaPath();

private:
    BottomBar* m_bottomBar;
};

#endif // PLAYLIST_H
