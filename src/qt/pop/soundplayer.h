// Ring-fork: Pop

#ifndef RING_QT_POP_SOUNDPLAYER_H
#define RING_QT_POP_SOUNDPLAYER_H

#define MA_NO_WASAPI
#define MA_NO_DSOUND

#include <qt/pop/miniaudio/dr_wav.h>
#include <qt/pop/miniaudio/miniaudio.h>

void JustPlay(QString filename);

#endif