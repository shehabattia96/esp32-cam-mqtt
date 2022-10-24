#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "./appProvider.h"
#include "./modules/wifi.h"
#include "./modules/mqtt.h"


void mqttHandlerTask(void *pvParameters)
{

    ESP_LOGI("mqttHandlerTask", "MQTT Handler Task started.");

    startMQTTClient();

    bool wifiIsConnected, mqttIsConnected;
    int lastSentFrameNumber = 0;

    while (1)
    {

    ESP_LOGI("mqttHandlerTask", "in MQTT Handler Task");
        vTaskDelay(5 / portTICK_RATE_MS);
        wifiIsConnected = (xEventGroupGetBits(wifiEventGroup) & WifiEvents::WIFI_CONNECTED_GROUPEVENT_FLAG);
        if (!wifiIsConnected)
        {
            ESP_LOGW("mqttHandlerTask", "MQTT is waiting for wifi to connect. MQTT client will auto-reconnect.");

            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        mqttIsConnected = (xEventGroupGetBits(mqttEventGroup) & MqttEvents::MQTT_CONNECTED_GROUPEVENT_FLAG);
        if (!mqttIsConnected)
        {
            ESP_LOGW("mqttHandlerTask", "MQTT is waiting to connect.");

            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        if (cameraFrame != nullptr && cameraFrame->len > 0)
        {
            ESP_LOGI("mqttHandlerTask", "cameraFrame is empty. Skipping mqtt publish.");

            continue;
        }

        if (frameNumber <= lastSentFrameNumber)
        {
            ESP_LOGD("mqttHandlerTask", "No new frames to send over mqtt. Last sent frame #%d", lastSentFrameNumber);
            continue;
        }

        esp_mqtt_client_publish(client, "camFeed", (char *)cameraFrame->buf, cameraFrame->len, 0, 0);

        lastSentFrameNumber = frameNumber;
        
    }

    // vTaskDelete( NULL );
}


TaskHandle_t mqttHandler = NULL;
void createMqttHandlerThread() {

    xTaskCreatePinnedToCore(
        mqttHandlerTask,
        "mqttTask",
        configMINIMAL_STACK_SIZE + 20000, // in words, not bytes
        NULL,
        1, // priority higher than tskIDLE_PRIORITY
        &mqttHandler,
        1 // run on core 1
    );

}