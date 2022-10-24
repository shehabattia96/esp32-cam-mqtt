#ifndef appProvider_H
#define appProvider_H

#include "esp_camera.h"

const int delay = 15;
int frameNumber = 0;

esp_err_t cameraInitStatus;
camera_fb_t *cameraFrame;


#endif