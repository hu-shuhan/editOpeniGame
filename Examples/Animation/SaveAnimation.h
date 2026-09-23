#pragma once

#include <FFMPEG/iGameFFMPEGVideoWriter.h>
#include <iGameScene.h>

// Shared by MP4 and GIF export. Use the first frame's range (or the caller's
// already locked range) throughout capture and restore the previous lock state.
iGame::VideoInputInfo CaptureAnimationFrames(iGame::Scene* scene, iGame::DataObject::Pointer object,
                                             int width = 1920, int height = 1080);
