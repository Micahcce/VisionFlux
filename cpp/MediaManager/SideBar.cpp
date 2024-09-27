#include "SideBar.h"

SideBar::SideBar(QWidget *parent) : QTabWidget(parent)
{
    setStyleSheet("background-color:#EEEEDD;");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(280);
}
