#include "VideocvPlugin.h"
#include "ffmpegplayer.h"
//=============================================================================
void VideocvPlugin::init(void)
{
  FfmpegPlayer::registerQmlType();
}
//=============================================================================
