#pragma once
#include "freertos/FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif
void vTaskDelay(TickType_t ticks);   /* PC 模拟器: 顺便把这一帧画到屏幕上 */
#ifdef __cplusplus
}
#endif