#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QTabWidget>

class SideBar : public QTabWidget
{
    Q_OBJECT
public:
    SideBar(QWidget *parent = nullptr);
};

#endif // SIDEBAR_H
