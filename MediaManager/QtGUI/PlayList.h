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


class PlayList : public QListWidget
{
    Q_OBJECT
public:
    explicit PlayList(QWidget *parent = nullptr);

    QString getVideoPath();
    void addMediaItem(const QString &thumbnailPath, const QString &title, const QString &duration, const QString &status);
};

#endif // PLAYLIST_H
