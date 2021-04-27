#ifndef mqtt_H
#define mqtt_H

// #include <SPI.h>
// #include <PubSubClient.h>

#include "mqtt_client.h"

#include "./.environment_variables.h"

#if DEBUG_MODE
#define TAG "mqtt"
#endif

esp_mqtt_client_handle_t client;

bool connectToMqttServer()
{
    esp_mqtt_client_config_t mqtt_cfg = {
      .host = mqttHost,
      .port = mqttPort,
      .client_id = mqttClientId
    };
    
    client = esp_mqtt_client_init(&mqtt_cfg);
   
    esp_err_t response = esp_mqtt_client_start(client);

    if (response != ESP_OK) {
        #if DEBUG_MODE
            ESP_LOGI(TAG, "Failed to start mqtt client. %d", response);
        #endif
        return false;
    }
    
    #if DEBUG_MODE
        ESP_LOGI(TAG, "Connected MQTT. %d", response);
    #endif
    return true;
}

#endif