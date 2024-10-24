#ifndef PROCESSPANEL_H
#define PROCESSPANEL_H

#include <QWidget>
#include <QScrollArea>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QUrl>
#include <QFileInfo>
#include "PlayController.h"

class ProcessPanel : public QScrollArea
{
    Q_OBJECT
public:
    explicit ProcessPanel(QWidget *parent = nullptr);

    void setPlayController(PlayController* playController) {m_playController = playController;}

signals:
    void sigLiveStreamPlay(QString streamUrl);

private slots:
    void slotLiveStreamSave();

    void slotPushStreamFileSelect();
    void slotPushStream();

    void slotConvertFileSelect();

    void slotAllEnd();

private:
    PlayController* m_playController;

    QLineEdit* m_pullStreamUrlEdit;
    QLineEdit* m_pushStreamUrlEdit;
    QLineEdit* m_pushStreamFileEdit;
    QLineEdit* m_convertFileEdit;
};

#endif // PROCESSPANEL_H
