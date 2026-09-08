#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Motor.h"

extern "C" void app_main()
{
    Motor pump(GPIO_NUM_25, GPIO_NUM_26);

    pump.forward(90);
    vTaskDelay(pdMS_TO_TICKS(5000));
    pump.setSpeed(50);
   
    

    printf("Running: %d\n", pump.isRunning());

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
