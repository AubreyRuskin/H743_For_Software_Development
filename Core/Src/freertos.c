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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "w25q64.h"
#include "lfs_port.h"
#include "lfs.h"
#include <string.h>

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
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  int ok = 0;

  /* --- Step 1: raw read block 0, check littlefs superblock magic --- */
  do {
    uint8_t hdr[64];
    uint32_t jedec_id = 0;
    if (W25Q64_ReadJedecId(&jedec_id) != W25Q64_OK || jedec_id != W25Q64_JEDEC_ID)
      break;
    if (W25Q64_EnableQuadMode() != W25Q64_OK)
      break;
    /* littlefs metadata tag for superblock contains "littlefs" string.
     * Scan the first 64 bytes for the magic. */
    if (W25Q64_Read(0, hdr, sizeof(hdr)) != W25Q64_OK)
      break;
    {
      int found = 0;
      for (size_t i = 0; i <= sizeof(hdr) - 8; i++) {
        if (memcmp(&hdr[i], "littlefs", 8) == 0) { found = 1; break; }
      }
      if (!found)
        break;
    }

    /* --- Step 2: mount via lfs_port (will NOT format if mount fails) --- */
    int mount_err = lfs_port_init();
    if (mount_err != 0)
      break;

    lfs_t *lfs = lfs_port_fs();
    if (lfs == NULL)
      break;

    /* --- Step 3: read /a.txt and verify content --- */
    lfs_file_t f;
    if (lfs_file_open(lfs, &f, "/a.txt", LFS_O_RDONLY) < 0)
      break;

    char buf[16];
    lfs_ssize_t n = lfs_file_read(lfs, &f, buf, sizeof(buf) - 1);
    lfs_file_close(lfs, &f);
    if (n < 4)
      break;
    buf[n] = '\0';

    if (strncmp(buf, "aaaa", 4) != 0)
      break;

    ok = 1;
  } while (0);

  if (!ok) {
    /* Verification failed: toggle LED once and stop */
    HAL_GPIO_TogglePin(GPIOH, GPIO_PIN_7);
  }

  /* Infinite loop */
  for(;;)
  {
    if (ok) {
      HAL_GPIO_TogglePin(GPIOH, GPIO_PIN_7);
    }
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
    /* 栈溢出时触发断点, 不关中断以保持 GDB 可调试 */
    (void)xTask;
    (void)pcTaskName;
    __asm volatile("bkpt #0");  /* 硬件断点, 调试器会停在这里 */
    for (;;) {}
}

/* USER CODE END Application */

