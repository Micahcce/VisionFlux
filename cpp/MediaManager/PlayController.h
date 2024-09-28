#ifndef PLAYCONTROLLER_H
#define PLAYCONTROLLER_H

#include "MediaManager.h"

class PlayController
{
public:
    PlayController();

    //开始播放
    //继续播放
    //暂停播放
    //结束播放

private:
    MediaManager* m_mediaManager;
};

#endif // PLAYCONTROLLER_H
