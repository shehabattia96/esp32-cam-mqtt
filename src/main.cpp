#include "./main.h"

void mainCameraHandlerLoop() {

    ESP_LOGI("mainCameraHandlerLoop", "Main camera loop started.");

    #if DEBUG
    static int64_t last_frame = esp_timer_get_time();
    #endif

    while (1)
    {
    ESP_LOGI("mainCameraHandlerLoop", "in Main camera loop.");
        vTaskDelay(2 / portTICK_RATE_MS);

        if (cameraInitStatus != ESP_OK)
        {
            ESP_LOGE("app_main", "Camera Init Failed.");
            continue;
        }

        cameraFrame = esp_camera_fb_get();
        #if DEBUG
        ESP_LOGI("app_main", "Size of cameraFrame: %zu bytes", cameraFrame->len);
        #endif

        esp_camera_fb_return(cameraFrame);
        frameNumber++;

        #if DEBUG

        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGD("app_main", "MJPG: %uKB %ums (%.1ffps)",
                    (uint32_t)(cameraFrame->len / 1024),
                    (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);

        #endif
    }

}

void app_main()
{

    initNvsFlash();

    // start camera
    cameraInitStatus = esp_camera_init(&camera_config);
    
    if (cameraInitStatus != ESP_OK)
    {
        ESP_LOGE("app_main", "Camera Init Failed.");
        return;
    }

    #if USE_MQTT // init wifi if using this flag
    registerWifiEventHandlers();
    initWifi();
    #endif

    #if USE_MQTT
    createMqttHandlerThread();
    #endif

    #if USE_SDCARD
    createSDCardHandlerThread();
    #endif

    resetCameraParameters();

    mainCameraHandlerLoop();
}