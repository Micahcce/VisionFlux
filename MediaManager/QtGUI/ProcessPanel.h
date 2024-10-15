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
#include "BottomBar.h"

class ProcessPanel : public QScrollArea
{
    Q_OBJECT
public:
    explicit ProcessPanel(QWidget *parent = nullptr);

    void setBottomBar(BottomBar* bottomBar) {m_bottomBar = bottomBar;}

private slots:
    void slotLiveStreamPlay();
    void slotLiveStreamSave();

    void slotPushStreamFileSelect();
    void slotPushStream();

    void slotConvertFileSelect();

    void slotAllEnd();

private:
    BottomBar* m_bottomBar;

    QLineEdit* m_pullStreamUrlEdit;
    QLineEdit* m_pushStreamUrlEdit;
    QLineEdit* m_pushStreamFileEdit;
    QLineEdit* m_convertFileEdit;
};

#endif // PROCESSPANEL_H
