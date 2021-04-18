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

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (esp_camera_init(&camera_config) != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed. Exiting.");
        return;
    }
    
    if (!setupWiFi()) {
        ESP_LOGE(TAG, "Could not connect to wifi access point. Exiting.");
        return;
    }
    
    if (!connectToMqttServer()) {
        ESP_LOGE(TAG, "Could not connect to mqtt server. Exiting.");
        return;
    }
            

    while (1)
    {
        
        camera_fb_t *pic = esp_camera_fb_get();

        #if DEBUG_MODE
        // use pic->buf to access the image
            ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
        #endif

        // mqttPublish("camFeed", pic->buf, pic->len);
        // client.publish("camFeed", pic->buf, pic->len);
        esp_mqtt_client_publish(client, "camFeed", (char*)pic->buf, pic->len, 0, 0);

        vTaskDelay(500 / portTICK_RATE_MS);
    }

    ESP_LOGI(TAG, "End Prog");
}