#include "stm32f7xx.h"
#include "stm32f7xx_hal_iwdg.h"

IWDG_HandleTypeDef hiwdg;
//WWDG_HandleTypeDef hiwdg;
/* Initializes the IWDG */
void WatchdogInit(void) {
    /* 
        Watchdog freq. is 40 kHz
        Prescaler: Min_Value = 4 and Max_Value = 256
        Reload: Min_Data = 0 and Max_Data = 0x0FFF
        TimeOut in seconds = (Reload * Prescaler) / Freq.
        MinTimeOut = (4 * 1) / 40000 = 0.0001 seconds (125 microseconds)
        MaxTimeOut = (256 * 4096) / 40000 = 26.2144 seconds
                
    */
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256; 
    hiwdg.Init.Reload = 1000;
    hiwdg.Init.Window = 2000;
    HAL_IWDG_Init(&hiwdg);
}
/* Refreshes the IWDG. */
void WatchdogRefresh(void) {
    HAL_IWDG_Refresh(&hiwdg);
}