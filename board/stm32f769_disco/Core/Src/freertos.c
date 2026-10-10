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
#include <stdbool.h>
#include <string.h>
#include "usart.h"        /* huart6 : liaison avec le pont Wi-Fi (Portenta) */
#include "queue.h"
#include "mqtt_topics.h"  /* table des topics : mqtt_topics_dispatch() */
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
static QueueHandle_t rxQueue;   /* caractères reçus sur USART6, de l'interruption vers la tâche */
static uint8_t rxByte;          /* la HAL y dépose chaque octet reçu */
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

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
  rxQueue = xQueueCreate(256, sizeof(uint8_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* MQTT par Ethernet coupé pour l'instant : le client (app/comms/mqtt/mqtt_client.c)
   * n'est ni compilé ni démarré. Les données arrivent seulement par le pont Wi-Fi. */

  /* Pont Wi-Fi (Portenta) : chaque ligne "topic valeur" reçue sur USART6 est
   * passée à la table des topics, comme le ferait un client MQTT. */
  char line[128];        /* ligne en cours de construction */
  size_t len = 0;
  bool overflow = false; /* ligne trop longue : on la jette entière jusqu'au '\n' */

  HAL_UART_Receive_IT(&huart6, &rxByte, 1);   /* lance la réception du premier octet */

  for(;;)
  {
    uint8_t c;
    if (xQueueReceive(rxQueue, &c, portMAX_DELAY) != pdTRUE)
      continue;

    if (c == '\r')
      continue;                               /* println() envoie "\r\n" : on ignore le '\r' */

    if (c == '\n')                            /* fin de ligne */
    {
      line[len] = '\0';
      char *space = memchr(line, ' ', len);   /* sépare "topic" et "valeur" au 1er espace */
      if (!overflow && space != NULL && space != line)
      {
        *space = '\0';                        /* la table attend un topic terminé par '\0' */
        size_t value_len = len - (size_t)(space - line) - 1;
        mqtt_topics_dispatch(line, space + 1, value_len);
      }
      HAL_GPIO_TogglePin(GPIOJ, GPIO_PIN_5); /* la LED change d'état à chaque ligne reçue */
      len = 0;
      overflow = false;
    }
    else if (len < sizeof(line) - 1)          /* - 1 : garder la place du '\0' */
    {
      line[len++] = (char)c;
    }
    else
    {
      overflow = true;                        /* jamais de ligne tronquée : valeur fausse */
    }
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* Appelée par la HAL, en interruption, quand l'octet demandé par HAL_UART_Receive_IT est arrivé */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart6)
  {
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(rxQueue, &rxByte, &woken);   /* en interruption : toujours la version FromISR */
    HAL_UART_Receive_IT(&huart6, &rxByte, 1);      /* relance la réception de l'octet suivant */
    portYIELD_FROM_ISR(woken);                     /* réveille la tâche tout de suite si elle attendait */
  }
}

/* Appelée par la HAL en cas d'erreur de réception (ex. overrun) : sans relance, la réception s'arrêterait */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart6)
  {
    HAL_UART_Receive_IT(&huart6, &rxByte, 1);
  }
}
/* USER CODE END Application */

