/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
/* send one byte (implemented in uart.c) */
extern int __io_putchar(int ch);

void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* stack overflow hook: the stack is ALREADY corrupted here.
    * Do NOT call printf/log (printf itself needs 400+ bytes of stack).
    * Only use __io_putchar or direct register writes. */
   const char *p;
   (void)xTask;

   p = "\r\n!!! STACK OVERFLOW in task [";
   while (*p != '\0') { __io_putchar(*p++); }

   if (pcTaskName != NULL) {
      while (*pcTaskName != '\0') { __io_putchar(*pcTaskName++); }
   }

   p = "] !!!\r\n";
   while (*p != '\0') { __io_putchar(*p++); }

   __disable_irq();
   for (;;)
   {
   }
}
/* USER CODE END 4 */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

