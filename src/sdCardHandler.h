#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string>

#include "./appProvider.h"
#include "./modules/sdcard.h"


// writes cameraFrames to SDCard
void fileWriterHandlerTask(void *pvParameters)
{
    if (!initSDCard())
        vTaskDelete(NULL);

    FILE *fout = NULL;

    char *newline = "\n";

#define MAX_FILE_SIZE 1000000 * 10 // 10mb

    int currentFileSize = 0;

    while (1)
    { // TODO: fflush every once in a while. TODO2: Check for available memory.
        if (fout == NULL || currentFileSize > MAX_FILE_SIZE)
        {
            if (fout != NULL)
                fclose(fout); // close t he file if it exceeds max file size.

            size_t fileCount = getNumberOfFiles(MOUNT_POINT "/");
            std::string filename = MOUNT_POINT "/file" + std::to_string(fileCount + 1) + ".bin";
            fout = fopen(filename.c_str(), "wb");

#if DEBUG
            if (fout == NULL)
                perror("Error opening file: ");
            else
                ESP_LOGD("fileWriterHandlerTask", "Opening new file %s.", filename.c_str());
#endif
        }
        if (fout != NULL)
        {
            if (cameraFrame != nullptr && cameraFrame->len > 0)
            {
#if DEBUG
                ESP_LOGD("fileWriterHandlerTask", "Writing cameraFrame to file..");
#endif
                size_t writtenLen = fwrite((char *)cameraFrame->buf, 1, cameraFrame->len, fout);
                fwrite(newline, 1, strlen(newline), fout); // add a known delimiter

#if DEBUG
                ESP_LOGD("fileWriterHandlerTask", "wrote to file: %d bytes. Expected: %zu bytes", writtenLen, cameraFrame->len);
#endif

                currentFileSize += cameraFrame->len;
            }
            else
            {
#if DEBUG
                ESP_LOGD("fileWriterHandlerTask", "cameraFrame is empty. Skipping writing to file.");
#endif
            }
        }
        else
        {
#if DEBUG
            ESP_LOGD("fileWriterHandlerTask", "fout is empty. Make sure SDCard is mounted.");
#endif
        }
        vTaskDelay(delay / portTICK_RATE_MS);
    }
}


TaskHandle_t fileWriterHandler = NULL;

void createSDCardHandlerThread() {

    xTaskCreatePinnedToCore(
        fileWriterHandlerTask,
        "fileWriterTask",
        configMINIMAL_STACK_SIZE + 20000, // in words, not bytes
        NULL,
        1, // priority higher than tskIDLE_PRIORITY
        &fileWriterHandler,
        1 // run on core 1
    );

}