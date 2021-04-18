#ifndef wifi_H
#define wifi_H
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "./.environment_variables.h"


#if DEBUG_MODE
#define TAG "wifi"
#endif


// WiFiClient wifiClient; // references https://github.com/witii/ArduinoMQTT/blob/master/examples/Hello/Hello.ino
// IPStack ipstack(wifiClient);


bool setupWiFi() {
    esp_err_t response;
    response = esp_netif_init();
    
    if (response != ESP_OK) {
        #if DEBUG_MODE
            ESP_LOGI(TAG, "Failed to initialize netif. ", response);
        #endif
        return false;
    }

    esp_netif_t *netifInstance = esp_netif_create_default_wifi_sta();

    #ifndef CONFIG_ESP32_WIFI_CACHE_TX_BUFFER_NUM
    #define CONFIG_ESP32_WIFI_CACHE_TX_BUFFER_NUM 0
    #endif
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    response = esp_wifi_init(&cfg);
    
    if (response != ESP_OK) {
        #if DEBUG_MODE
            ESP_LOGI(TAG, "Failed to init Wifi module. ", response);
        #endif
        return false;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = (uint8_t)wifiSsid,
            .password = (uint8_t)wifiPassword,
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    response = esp_wifi_set_mode(WIFI_MODE_STA);
    response = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    response = esp_wifi_start();
    
    if (response != ESP_OK) {
        #if DEBUG_MODE
            ESP_LOGI(TAG, "Failed to connect to Access Point ", response);
        #endif
        return false;
    }

    esp_netif_ip_info_t ipInfo;
    response = esp_netif_get_ip_info(netifInstance, &ipInfo);
    
    if (response != ESP_OK) {
        #if DEBUG_MODE
            ESP_LOGI(TAG, "Failed to get IP information. %d", response);
        #endif
        return false;
    }
    
    #if DEBUG_MODE
        ESP_LOGI(TAG, "IP address: ", ipInfo.ip.addr);
    #endif
    

    // WiFi.begin(ssid, password);
    // while ( WiFi.status() != WL_CONNECTED) {
    //     delay(300);
    // }

    // #if DEBUG_MODE
    //     ESP_LOGI(TAG, "Connected to network. Waiting for IP.");
    // #endif

    // while (WiFi.localIP() == INADDR_NONE) {
    //     delay(300);
    // }
    
    // #if DEBUG_MODE
    //     ESP_LOGI(TAG, "IP address: ", WiFi.localIP());
    //     ESP_LOGI(TAG, "Signal Strength: ", WiFi.RSSI());
    // #endif
    return true;
}

#endif