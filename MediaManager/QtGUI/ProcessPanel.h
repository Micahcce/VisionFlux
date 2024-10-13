#ifndef PROCESSPANEL_H
#define PROCESSPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class ProcessPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ProcessPanel(QWidget *parent = nullptr);
};

#endif // PROCESSPANEL_H
