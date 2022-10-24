#ifndef mqtt_H
#define mqtt_H

#include "freertos/event_groups.h"
#include <esp_log.h>

#include "mqtt_client.h" //espidf module

#include "./.environment_variables.h"

EventGroupHandle_t mqttEventGroup;
esp_mqtt_client_handle_t client;

enum MqttEvents {
    MQTT_DISCONNECTED_GROUPEVENT_FLAG = BIT1,
    MQTT_CONNECTED_GROUPEVENT_FLAG = BIT2,
};

esp_err_t mqttEvent(esp_mqtt_event_t* event) {

  #if DEBUG
      ESP_LOGD("MQTT", "Received MQTT Event. %d", event->event_id);
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
    mqttEventGroup = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg {
      .event_handle = mqttEvent,
      .host = mqttHost,
      .port = mqttPort,
      .client_id = mqttClientId,
      .username = mqttUser,
      .password = mqttPassword
      // .buffer_size = 9000
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
   
    ESP_LOGI("MQTT", "Start MQTT Client");

    esp_mqtt_client_start(client);
}

#endif