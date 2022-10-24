#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_spiram.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"

#include "esp_camera.h"

#include "./main.h"
#include "./.environment_variables.h"
#include "./cameraConfig.h"
#include "./wifi.h"
#include "./mqtt.h"
#include "./sdcard.h"

#if DEBUG
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
int delay = 15;
int frameNumber = 0;
// SemaphoreHandle_t cameraFrameSemaphore = xSemaphoreCreateCounting( 2, 0 );
esp_err_t cameraInitStatus;
camera_fb_t *cameraFrame;
void mqttHandlerTask( void *pvParameters )
{
    #if DEBUG
        ESP_LOGE(TAG, "Starting MQTT Client");
    #endif
    // config and start mqtt client
    startMQTTClient();

    #if DEBUG
        ESP_LOGE(TAG, "Starting MQTT Loop");
    #endif
    bool wifiIsConnected, mqttIsConnected;
    int lastSentFrameNumber = 0;
    
    int64_t last_frame = esp_timer_get_time();
    while( 1 )
    {
        wifiIsConnected = (xEventGroupGetBits(wifiEventGroup) & WifiEvents::WIFI_CONNECTED_GROUPEVENT_FLAG);
        if (wifiIsConnected) {
            mqttIsConnected = (xEventGroupGetBits(mqttEventGroup) & MqttEvents::MQTT_CONNECTED_GROUPEVENT_FLAG);
            if (mqttIsConnected) {
                if (cameraFrame != nullptr && cameraFrame->len > 0) {

                    if (frameNumber > lastSentFrameNumber) {
                    // xSemaphoreTake(cameraFrameSemaphore, portMAX_DELAY);
                    esp_mqtt_client_publish(client, "camFeed", (char*)cameraFrame->buf, cameraFrame->len, 0, 0);
                    // xSemaphoreGive(cameraFrameSemaphore);
                    lastSentFrameNumber = frameNumber;
                    
                    #if DEBUG

                        int64_t fr_end = esp_timer_get_time();
                        int64_t frame_time = fr_end - last_frame;
                        last_frame = fr_end;
                        frame_time /= 1000;
                        ESP_LOGI(TAG, "last mqtt: %uKB %ums (%.1ffps)",
                            (uint32_t)(cameraFrame->len/1024),
                            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);

                    #endif
                    } else {
                        
                    #if DEBUG
                        ESP_LOGI(TAG, "No new frames to send over mqtt. Last sent frame #%d", lastSentFrameNumber);
                    #endif
                    }
                }
                else {
                    #if DEBUG
                        ESP_LOGI(TAG, "cameraFrame is empty. Skipping mqtt publish.");
                    #endif
                }
            } else {
                #if DEBUG
                    ESP_LOGI(TAG, "MQTT is waiting to connect.");
                #endif
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
        } else {
            #if DEBUG
                ESP_LOGI(TAG, "MQTT is waiting for wifi to connect. MQTT client will auto-reconnect.");
            #endif
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
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
            if (fout != NULL) fclose(fout); // close t he file if it exceeds max file size.

            size_t fileCount = getNumberOfFiles(MOUNT_POINT"/");
            std::string filename = MOUNT_POINT"/file"+std::to_string(fileCount+1)+".bin";
            fout = fopen(filename.c_str(), "wb");
            
            #if DEBUG
                if (fout == NULL)
                    perror("Error opening file: "  );
                else
                    ESP_LOGI(TAG, "Opening new file %s.", filename.c_str());
            #endif
        }
        if (fout != NULL) {
            if (cameraFrame != nullptr && cameraFrame->len > 0) {
                #if DEBUG
                    ESP_LOGI(TAG, "Writing cameraFrame to file..");
                #endif
                size_t writtenLen = fwrite((char*)cameraFrame->buf, 1,cameraFrame->len,fout);
                fwrite(newline, 1, strlen(newline), fout); // add a known delimiter
                
                #if DEBUG
                    ESP_LOGI(TAG, "wrote to file: %d bytes. Expected: %zu bytes", writtenLen, cameraFrame->len);
                #endif

                currentFileSize+=cameraFrame->len;
            }
            else {
                #if DEBUG
                    ESP_LOGI(TAG, "cameraFrame is empty. Skipping writing to file.");
                #endif
            }
        } else {
            #if DEBUG
                ESP_LOGI(TAG, "fout is empty. Make sure SDCard is mounted.");
            #endif
        }
        vTaskDelay(delay / portTICK_RATE_MS);
    }
}


void app_main()
{

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

    initNvsFlash();

    // start camera
    cameraInitStatus = esp_camera_init(&camera_config);
    if (cameraInitStatus != ESP_OK) {
        #if DEBUG
        ESP_LOGE(TAG, "Camera Init Failed.");
            #endif
    } else {

        // reset camera params
        sensor_t * s = esp_camera_sensor_get();
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
        s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
        s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
        s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
        s->set_aec2(s, 0);           // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
        s->set_bpc(s, 0);            // 0 = disable , 1 = enable
        s->set_wpc(s, 1);            // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
        s->set_lenc(s, 1);           // 0 = disable , 1 = enable
        s->set_hmirror(s, 1);        // 0 = disable , 1 = enable
        s->set_vflip(s, 1);          // 0 = disable , 1 = enable
        s->set_dcw(s, 1);            // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

    }

    // config and start wifi
    registerWifiEventHandlers(); // this allocates the default event queue loop
    initWifi();

    
    #if DEBUG
        ESP_LOGE(TAG, "Creating MQTT handler task");
    #endif
    TaskHandle_t mqttHandler = NULL;
    xTaskCreatePinnedToCore(
        mqttHandlerTask,       /* Function that implements the task. */
        "mqttTask",          /* Text name for the task. */
        configMINIMAL_STACK_SIZE+20000,      /* Stack size in words, not bytes. */
        NULL,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY+1,/* Priority at which the task is created. */
        &mqttHandler, /* Used to pass out the created task's handle. */
        1
    );    
    #if DEBUG
        ESP_LOGE(TAG, "Finished creating MQTT handler task");
    #endif

    // TaskHandle_t fileWriterHandler = NULL;
    // xTaskCreate(
    //     fileWriterHandlerTask,       /* Function that implements the task. */
    //     "fileWriterTask",          /* Text name for the task. */
    //     configMINIMAL_STACK_SIZE+20000,      /* Stack size in words, not bytes. */
    //     NULL,    /* Parameter passed into the task. * /
    //     tskIDLE_PRIORITY+5,/* Priority at which the task is created. */
    //     &fileWriterHandler /* Used to pass out the created task's handle. */
    // );     
    static int64_t last_frame = esp_timer_get_time();

    while (1)
    {      
          
        if (cameraInitStatus == ESP_OK) {
            cameraFrame = esp_camera_fb_get();
            #if DEBUG
                ESP_LOGI(TAG, "Size of cameraFrame: %zu bytes", cameraFrame->len);
            #endif

            esp_camera_fb_return(cameraFrame);
            frameNumber++;
            
            #if DEBUG

                int64_t fr_end = esp_timer_get_time();
                int64_t frame_time = fr_end - last_frame;
                last_frame = fr_end;
                frame_time /= 1000;
                ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
                    (uint32_t)(cameraFrame->len/1024),
                    (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);

            #endif
        } else {
            #if DEBUG
                ESP_LOGW(TAG, "Camera didn't initialize correctly.");
            #endif
            
            last_frame = esp_timer_get_time();

            vTaskDelay(delay / portTICK_RATE_MS);
        }

    }
}