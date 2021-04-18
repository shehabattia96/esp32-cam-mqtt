#ifndef mqtt_H
#define mqtt_H

// #include <SPI.h>
// #include <PubSubClient.h>

#include "mqtt_client.h"

#include "./.environment_variables.h"
#include "./wifi.h"

#if DEBUG_MODE
#define TAG "mqtt"
#endif


// void messageArrived(MQTT::MessageData& md)
// {
//   MQTT::Message &message = md.message;
  
//   Serial.print("Message ");
//   Serial.print(++arrivedcount);
//   Serial.print(" arrived: qos ");
//   Serial.print(message.qos);
//   Serial.print(", retained ");
//   Serial.print(message.retained);
//   Serial.print(", dup ");
//   Serial.print(message.dup);
//   Serial.print(", packetid ");
//   Serial.println(message.id);
//   Serial.print("Payload ");
//   Serial.println((char*)message.payload);
// }

// PubSubClient client(ipstack);
// PubSubClient client(wifiClient);

// byte mac[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };  // replace with your device's MAC

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
            ESP_LOGI(TAG, "Failed to start mqtt client. ", response);
        #endif
        return false;
    }
  // client.setServer(mqttHost, mqttPort);
  // client.setCallback(callback);
  
  // if (!client.connect(mqttClientId)) {
  //   #if DEBUG_MODE
  //       ESP_LOGE(TAG, "Failed to reach MQTT server. Response code: ", client.state());
  //   #endif
  //   return false;
  // }
//   int rc = ipstack.connect(mqttHost, mqttPort);
//   if (rc != 1)
//   {
//     #if DEBUG_MODE
//         ESP_LOGE(TAG, "Failed to reach MQTT server. Response code: ", rc);
//     #endif
//     return false;
//   }
 
//   MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
//   data.MQTTVersion = 5;
//   data.clientID.cstring = mqttClientId;
//   rc = client.connect(data);
//   if (rc != 0)
//   {
//     #if DEBUG_MODE
//         ESP_LOGE(TAG, "Failed to connect to MQTT server. Response code: ", rc);
//     #endif
//     return false;
  // }
  
//   const char* topic = "arduino-sample";
//   rc = client.subscribe(topic, MQTT::QOS0, messageArrived);   
//   if (rc != 0)
//   {
//     #if DEBUG_MODE
//         ESP_LOGE(TAG, "Failed to subscribe to topic. Response code: ", rc);
//     #endif
//   }
    return true;
}

// void mqttPublish(const char* topic, const uint8_t* payload, const int payloadLength)
// { 
// //   MQTT::Message message;
// //   message.qos = MQTT::QOS0;
// //   message.retained = false;
// //   message.dup = false;
// //   message.payload = payload;
// //   message.payloadlen = payloadLength;
// //   client.publish(topic, message);
// }

#endif