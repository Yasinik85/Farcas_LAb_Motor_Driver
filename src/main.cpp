#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Motor.h"
#include "wifi_ap.h"
#include "web_server.h"

extern "C" void app_main()
{
    static Motor pump(GPIO_NUM_25, GPIO_NUM_26);

    wifi_ap_start();      // ESP32 creates its own WiFi hotspot
    web_server_start(&pump); // serves the GUI + control API over that hotspot

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
