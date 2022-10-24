#ifndef main_H
#define main_H
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_camera.h"

#include "./.environment_variables.h"
#include "./appProvider.h"

#include "./cameraConfig/cameraConfig.h"

#define USE_MQTT true
#define USE_SDCARD false

#if USE_MQTT
#include "./mqttHandler.h"
#endif

#if USE_SDCARD
#include "./sdCardHandler.h"
#endif


void initNvsFlash()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

extern "C" {
	void app_main(void);
}

#endif