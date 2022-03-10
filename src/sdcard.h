#ifndef sdcard_H
#define sdcard_H

#include <stdio.h>

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include "./.environment_variables.h"

#if DEBUG
#define TAG "sdcard.h"
#endif

#define MOUNT_POINT "/sdcard"

// references https://stackoverflow.com/a/12506/9824103
size_t getNumberOfFiles(char* directory) {
    size_t numberOfFiles = 0;
    DIR* dir = opendir(directory);
    if (dir != NULL)
    {
        dirent* file = readdir(dir);
        while (file) {
            #if DEBUG
                ESP_LOGE(TAG, "Filename: %s.", file->d_name);
            #endif
            numberOfFiles++;
            file = readdir(dir);
        }

        closedir(dir);
    }

    return numberOfFiles;
}

bool sdCardInitialized = false;

bool initSDCard()
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t* card;
    // const char mount_point[] = MOUNT_POINT;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        #if DEBUG
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount filesystem.");
            } else {
                ESP_LOGE(TAG, "Failed to initialize the card (%s). ", esp_err_to_name(ret));
            }
        #endif
        return false;
    }

    #if DEBUG
        sdmmc_card_print_info(stdout, card);
    #endif

    return true;

    // // All done, unmount partition and disable SDMMC or SPI peripheral
    // esp_vfs_fat_sdcard_unmount(mount_point, card);
    // ESP_LOGI(TAG, "Card unmounted");
}

#endif