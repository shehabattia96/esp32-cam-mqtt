#include "./main.h"
#include "./.environment_variables.h"
#include "./cameraConfig.h"
#include "./wifi.h"
#include "./mqtt.h"

#if DEBUG_MODE
#define TAG "main.cpp"
#endif

extern "C" {
	void app_main(void);
}

void initNvsFlash() {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

esp_err_t cameraInitStatus;
camera_fb_t *cameraFrame;
void mqttHandlerTask( void *pvParameters )
{
    // config and start mqtt client
    startMQTTClient();

    bool wifiIsConnected;
    while( 1 )
    {
        wifiIsConnected = (xEventGroupGetBits(wifiEventGroup) & WIFI_CONNECTED_GROUPEVENT_FLAG);
        if (wifiIsConnected) {
            if (cameraFrame != nullptr && cameraFrame->len > 0) {
                #if DEBUG_MODE
                    ESP_LOGI(TAG, "Going to send %zu bytes over MQTT", cameraFrame->len);
                #endif
                esp_mqtt_client_publish(client, "camFeed", (char*)cameraFrame->buf, cameraFrame->len, 0, 0);
            }
            else {
                #if DEBUG_MODE
                    ESP_LOGI(TAG, "cameraFrame is empty. Skipping mqtt publish.");
                #endif
            }
        } else {
            #if DEBUG_MODE
                ESP_LOGI(TAG, "MQTT is waiting for wifi to connect. MQTT client will auto-reconnect.");
            #endif
        }
        vTaskDelay(500 / portTICK_RATE_MS);
    }

    // vTaskDelete( NULL );
}


void app_main()
{
    initNvsFlash();

    // start camera
    cameraInitStatus = esp_camera_init(&camera_config);
    if (cameraInitStatus != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed.");
    }
    
    // config and start wifi
    registerWifiEventHandlers(); // this allocates the default event queue loop
    initWifi();

    
    TaskHandle_t mqttHandler = NULL;
    xTaskCreate(
        mqttHandlerTask,       /* Function that implements the task. */
        "mqttTask",          /* Text name for the task. */
        configMINIMAL_STACK_SIZE+20000,      /* Stack size in words, not bytes. */
        NULL,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+5,/* Priority at which the task is created. */
        &mqttHandler /* Used to pass out the created task's handle. */
    );     
            
    while (1)
    {        
        if (cameraInitStatus == ESP_OK) {
            cameraFrame = esp_camera_fb_get();
            #if DEBUG_MODE
                ESP_LOGI(TAG, "Size of cameraFrame: %zu bytes", cameraFrame->len);
            #endif
        } else {
            #if DEBUG_MODE
                ESP_LOGW(TAG, "Camera didn't initialize correctly.");
            #endif
        }

        vTaskDelay(500 / portTICK_RATE_MS);
    }
}