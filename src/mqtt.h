#ifndef mqtt_H
#define mqtt_H

#include "mqtt_client.h" //espidf module

#include "./.environment_variables.h"

#if DEBUG
#define TAG "mqtt"
#endif

EventGroupHandle_t mqttEventGroup;
esp_mqtt_client_handle_t client;

enum MqttEvents {
    MQTT_DISCONNECTED_GROUPEVENT_FLAG = BIT1,
    MQTT_CONNECTED_GROUPEVENT_FLAG = BIT2,
};

esp_err_t mqttEvent(esp_mqtt_event_t* event) {

  #if DEBUG
      ESP_LOGE(TAG, "Received MQTT Event. %d", event->event_id);
  #endif
  
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    xEventGroupSetBits(mqttEventGroup, MqttEvents::MQTT_CONNECTED_GROUPEVENT_FLAG);
    break;
  case MQTT_EVENT_DISCONNECTED:
    xEventGroupSetBits(mqttEventGroup, MqttEvents::MQTT_DISCONNECTED_GROUPEVENT_FLAG);
    break;
  default:
    break;
  }
    return ESP_OK;
}

void startMQTTClient()
{
  
    #if DEBUG
        ESP_LOGE(TAG, "Creating event group");
    #endif
    mqttEventGroup = xEventGroupCreate();

    #if DEBUG
        ESP_LOGE(TAG, "MQTT Client config");
    #endif
    esp_mqtt_client_config_t mqtt_cfg {
      .event_handle = mqttEvent,
      .host = mqttHost,
      .port = mqttPort,
      .client_id = mqttClientId,
      .username = mqttUser,
      .password = mqttPassword
      // .buffer_size = 9000
    };
    // mqtt_cfg.host = mqttHost;
    // mqtt_cfg.port = mqttPort;
    // mqtt_cfg.client_id = mqttClientId;
    // mqtt_cfg.event_handle = &mqttEvent;
    
    #if DEBUG
        ESP_LOGE(TAG, "Init MQTT Client");
    #endif
    client = esp_mqtt_client_init(&mqtt_cfg);
   
    #if DEBUG
        ESP_LOGE(TAG, "Start MQTT Client");
    #endif
    esp_mqtt_client_start(client);
}

#endif