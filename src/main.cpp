#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_camera.h"

#include "./main.h"
#include "./.environment_variables.h"
#include "./cameraConfig.h"
#include "./wifi.h"
#include "./mqtt.h"
#include "./sdcard.h"

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

// writes cameraFrames to SDCard
void fileWriterHandlerTask( void *pvParameters )
{
    if (!initSDCard()) vTaskDelete( NULL );
    
    FILE* fout = NULL;

    char* newline="\n";

    #define MAX_FILE_SIZE 1000000*10 //10mb
    
    int currentFileSize = 0;

    while( 1 )
    { // TODO: fflush every once in a while. TODO2: Check for available memory.
        if (fout == NULL || currentFileSize > MAX_FILE_SIZE) {
            if (fout != NULL) fclose(fout); // close the file if it exceeds max file size.

            size_t fileCount = getNumberOfFiles(MOUNT_POINT"/");
            std::string filename = MOUNT_POINT"/file"+std::to_string(fileCount+1)+".bin";
            fout = fopen(filename.c_str(), "wb");
            
            #if DEBUG_MODE
                if (fout == NULL)
                    perror("Error opening file: ");
                else
                    ESP_LOGI(TAG, "Opening new file %s.", filename.c_str());
            #endif
        }
        if (fout != NULL) {
            if (cameraFrame != nullptr && cameraFrame->len > 0) {
                #if DEBUG_MODE
                    ESP_LOGI(TAG, "Writing cameraFrame to file..");
                #endif
                size_t writtenLen = fwrite((char*)cameraFrame->buf, 1,cameraFrame->len,fout);
                fwrite(newline, 1, strlen(newline), fout); // add a known delimiter
                
                #if DEBUG_MODE
                    ESP_LOGI(TAG, "wrote to file: %d bytes. Expected: %zu bytes", writtenLen, cameraFrame->len);
                #endif

                currentFileSize+=cameraFrame->len;
            }
            else {
                #if DEBUG_MODE
                    ESP_LOGI(TAG, "cameraFrame is empty. Skipping writing to file.");
                #endif
            }
        } else {
            #if DEBUG_MODE
                ESP_LOGI(TAG, "fout is empty. Make sure SDCard is mounted.");
            #endif
        }
        vTaskDelay(500 / portTICK_RATE_MS);
    }
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

    TaskHandle_t fileWriterHandler = NULL;
    xTaskCreate(
        fileWriterHandlerTask,       /* Function that implements the task. */
        "fileWriterTask",          /* Text name for the task. */
        configMINIMAL_STACK_SIZE+20000,      /* Stack size in words, not bytes. */
        NULL,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+5,/* Priority at which the task is created. */
        &fileWriterHandler /* Used to pass out the created task's handle. */
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