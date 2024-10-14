#ifndef PROCESSPANEL_H
#define PROCESSPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "BottomBar.h"

class ProcessPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ProcessPanel(QWidget *parent = nullptr);

    void setBottomBar(BottomBar* bottomBar) {m_bottomBar = bottomBar;}

private slots:
    void slotLiveStreamPlay();

private:
    BottomBar* m_bottomBar;

    QLineEdit* m_streamUrlEdit;
};

#endif // PROCESSPANEL_H
