/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEBOUNCE_MS 200
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

typedef enum
{
	sPomodoro = 0,
	sPause,
	sPlay
}bstate;

bstate pomoState = sPause;
TIM_HandleTypeDef htim10;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart6_rx;

/* USER CODE BEGIN PV */
TaskHandle_t menu_task_handle;
TaskHandle_t print_task_handle;
TaskHandle_t AT_task_handle;
TaskHandle_t cmd_task_handle;
TaskHandle_t pomo_view_handle;
TaskHandle_t pomodoro_handle;
QueueHandle_t q_print;
BaseType_t status;
uint8_t uart2_data[2048];
uint8_t uart6_data[2048];

//uint8_t uart6_data[2048];
uint8_t uart2;
uint8_t uart6;
uint16_t size2;
uint16_t size6;

unsigned int seconds = 0;
unsigned int minute = 0;
unsigned int counter = 0;
unsigned int temp_second = 0;
unsigned int temp_minute = 0;
UART_HandleTypeDef uart;
SemaphoreHandle_t uart2_mutex;

int button_state = 0;
uint32_t lastSeen = 0;
uint32_t lastSeen_l = 0;
int count = 0;


typedef enum
{
	sMainMenu = 0,
	sAt_cmd,
	sPomodorView

}state;
state currState = sMainMenu;
typedef struct
{
	uint8_t payload[2048];
	uint16_t len;
}command_t;
typedef enum
{
	awaitingNone = 0,
	awaitingCwmode,
	awaitingCwset,
	awaitingAIResponse
} awaiting_response_t;

awaiting_response_t awaitingResponse = awaitingNone;


typedef enum
{
	wifiIdle = 0,
	wifiAwaitingSSID,
	wifiAwaitingPassword
} wifi_setup_state_t;

uint8_t setMode = 0;
typedef enum
{
	aiIdle = 0,
	aiAwaitingQuestion
} ai_state_t;

ai_state_t aiState = aiIdle;


wifi_setup_state_t wifiState = wifiIdle;

char wifi_ssid[64];
char wifi_pass[64];


char cmd_line[128];
uint16_t cmd_idx = 0;
char cmd[] = "AT+CWMODE?\r\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_TIM10_Init(void);
/* USER CODE BEGIN PFP */
void menu_task_handler(void *parameter);
void print_task_handler(void *parameter);
void cmd_task_handler(void *parameter);
void AT_task_handler(void *parameter);
void pomodoro_view_handler(void *parameter);
void pomodoro_handler_func(void *parameter);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */
  status = xTaskCreate(menu_task_handler,"main menu",200,NULL,2,&menu_task_handle);
  configASSERT(status == pdPASS);

  status = xTaskCreate(print_task_handler,"print task",200,NULL,2,&print_task_handle);
  configASSERT(status == pdPASS);

  status = xTaskCreate(cmd_task_handler,"print task",200,NULL,2,&cmd_task_handle);
  configASSERT(status == pdPASS);

  status = xTaskCreate(AT_task_handler,"AT task",200,NULL,2,&AT_task_handle);
  configASSERT(status == pdPASS);

  status = xTaskCreate(pomodoro_view_handler,"pomo task",200,NULL,2,&pomo_view_handle);
  configASSERT(status == pdPASS);

  status = xTaskCreate(pomodoro_handler_func,"Pomodoro clock",200,NULL,2,&pomodoro_handle);
    configASSERT(status == pdPASS);

  q_print = xQueueCreate(10,sizeof(char*));


  uart2_mutex = xSemaphoreCreateMutex();
  configASSERT(uart2_mutex != NULL);

  HAL_UARTEx_ReceiveToIdle_DMA(&huart2,uart2_data,2048);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart6,uart6_data,2048);


  vTaskStartScheduler();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 99;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 9999;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart == &huart2)
	{

		uart2 = 1;
		  HAL_UARTEx_ReceiveToIdle_DMA(&huart2,uart2_data,2048);
		  size2 = Size;
	}
	else if(huart == &huart6)
	{
		  uart6 = 1;
		  HAL_UARTEx_ReceiveToIdle_DMA(&huart6,uart6_data,2048);
		  size6 = Size;

	}
		else
		{
			HAL_UART_Transmit(&huart2, uart6_data, size6, HAL_MAX_DELAY);
		}
	}



void cmd_task_handler(void *parameter)
{
	while(1)
	{
		if(uart2)
		{
			uart2 = 0;
			switch(currState)
			{
			case sMainMenu:
			{
				if(uart2_data[0] == '0')
				{
					xTaskNotify(AT_task_handle,0,eNoAction);
					currState = sAt_cmd;
				}
				else if(uart2_data[0] == '1')
				{
					xTaskNotify(pomo_view_handle,0,eNoAction);
					currState = sPomodorView;
				}
				break;
			}
			case sAt_cmd:
			{
				for (uint16_t i = 0; i < size2; i++)
				{
					uint8_t b = uart2_data[i];

					if (b == '\r' || b == '\n')
					{
						if (cmd_idx > 0)
						{
							cmd_line[cmd_idx] = '\0';  // null-terminate for strcmp

							if(wifiState == wifiAwaitingSSID)
							{
								strncpy(wifi_ssid, cmd_line, sizeof(wifi_ssid) - 1); //memset repeatedly copies the same byte to a structure but strncpy copies exact same character to from src to the dest till it hits '\0'
								wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
								printf("Enter Password: \r\n");
								wifiState = wifiAwaitingPassword;
							}
							else if (wifiState == wifiAwaitingPassword)
							{
								strncpy(wifi_pass, cmd_line, sizeof(wifi_pass) - 1);
								wifi_pass[sizeof(wifi_pass) - 1] = '\0';

								char wifi_cmd[160];
								int n = snprintf(wifi_cmd, sizeof(wifi_cmd),
								                  "AT+CWJAP=\"%s\",\"%s\"\r\n", wifi_ssid, wifi_pass);
								HAL_UART_Transmit(&huart6, (uint8_t*)wifi_cmd, n, HAL_MAX_DELAY);
								wifiState = wifiIdle;   // back to normal AT-command behavior

							}
							else if (aiState == aiAwaitingQuestion)          // <-- ADD this new block here
							{
								aiState = aiIdle;

								for (uint16_t j = 0; cmd_line[j] != '\0'; j++)
								{
									if (cmd_line[j] == ' ') cmd_line[j] = '+';
								}

								char http_cmd[256];
								int n = snprintf(http_cmd, sizeof(http_cmd),
								    "AT+HTTPCGET=\"http://192.168.1.23:5000/ask?question=%s\",2048,2048,60000\r\n",
								    cmd_line);

								awaitingResponse = awaitingAIResponse;
								HAL_UART_Transmit(&huart6, (uint8_t*)http_cmd, n, HAL_MAX_DELAY);
							}
						    else if (strcmp(cmd_line, "exit") == 0)
							{
								currState = sMainMenu;
								xTaskNotify(menu_task_handle, 0, eNoAction);  // wake menu task to reprint
							}
							else if(strcmp(cmd_line,"EXIT") == 0)
							{
								currState = sMainMenu;
								xTaskNotify(menu_task_handle, 0, eNoAction);
							}
							else if(strcmp(cmd_line,"0") == 0)
							{
								printf("\r\n");
								awaitingResponse = awaitingCwmode;
								HAL_UART_Transmit(&huart6,(uint8_t *)cmd,strlen(cmd),HAL_MAX_DELAY);
							}
							else if(setMode)
							{
								if(strcmp(cmd_line,"1") == 0)
								{
									char cmd_mode[] = "AT+CWMODE=1\r\n";
									HAL_UART_Transmit(&huart6,(uint8_t*)cmd_mode,strlen(cmd_mode),HAL_MAX_DELAY);
									setMode = 0;
								}
									else if(strcmp(cmd_line,"2") == 0)
								{
									char cmd_mode[] = "AT+CWMODE=2\r\n";
									HAL_UART_Transmit(&huart6,(uint8_t*)cmd_mode,strlen(cmd_mode),HAL_MAX_DELAY);
									setMode = 0;
								}
									else if(strcmp(cmd_line,"3") == 0)
									{
										char cmd_mode[] = "AT+CWMODE=3\r\n";
										HAL_UART_Transmit(&huart6,(uint8_t*)cmd_mode,strlen(cmd_mode),HAL_MAX_DELAY);
										setMode = 0;
									}
							}
							else if(strcmp(cmd_line,"1") == 0)
							{
								//change the working mode
								char* msg_mode = "station mode-      ->    --> 1\r\n"
										         "Access point mode -->    --> 2\r\n"
										         "Dual mode(station_access)-->3\r\n"
									             "Enter your choice        --> ";
								xQueueSend(q_print,&msg_mode,portMAX_DELAY);
								setMode = 1;
							}

							else if(strcmp(cmd_line,"2") == 0)
							{


								printf("\r\n");
								printf("Enter SSID: \r\n");
								wifiState = wifiAwaitingSSID;

							}
							else if(strcmp(cmd_line,"4") == 0)
							{
								printf("\r\n");
								printf("Enter your question: \r\n");
								aiState = aiAwaitingQuestion;
							}
							else
							{
								cmd_line[cmd_idx++] = '\r';
								cmd_line[cmd_idx++] = '\n';
								HAL_UART_Transmit(&huart6, (uint8_t*)cmd_line, cmd_idx, HAL_MAX_DELAY);
							}
							cmd_idx = 0;
						}
					}
					else if (cmd_idx < sizeof(cmd_line) - 2)
					{
						cmd_line[cmd_idx++] = b;
					}

				}
				break;
			}
			case sPomodorView:
			{
				for (uint16_t i = 0; i < size2; i++)
				{
					uint8_t b = uart2_data[i];
					if (b == '\r' || b == '\n')
					{
						if (cmd_idx > 0)
						{
							cmd_line[cmd_idx] = '\0';  // null-terminate for strcmp
							if (strcmp(cmd_line, "exit") == 0)
							{
								currState = sMainMenu;
								xTaskNotify(menu_task_handle, 0, eNoAction);  // wake menu task to reprint
							}
							else if(strcmp(cmd_line,"EXIT") == 0)
							{
								currState = sMainMenu;
								xTaskNotify(menu_task_handle, 0, eNoAction);

							}
							cmd_idx = 0;
						}
						}
						else if (cmd_idx < sizeof(cmd_line) - 2)
						{
							cmd_line[cmd_idx++] = b;
						}
						}
				break;
			}
			}
		}
		else if(uart6)
		{
			uart6 = 0;
			uart6_data[size6] = '\0';

			if (awaitingResponse == awaitingCwmode)
			{
				awaitingResponse = awaitingNone;
				if (strstr((char*)uart6_data, "+CWMODE:1"))
					printf("current mode of working is: Station mode\r\n");
				else if (strstr((char*)uart6_data, "+CWMODE:2"))
					printf("current mode of working is: SoftAP mode\r\n");
				else if (strstr((char*)uart6_data, "+CWMODE:3"))
					printf("current mode of working is: Station + SoftAP mode\r\n");
				else
					printf("could not determine current mode\r\n");
				HAL_UART_Transmit(&huart2, uart6_data, size6, HAL_MAX_DELAY);
			}
			else if (awaitingResponse == awaitingAIResponse)
			{
				awaitingResponse = awaitingNone;
				char *body = strstr((char*)uart6_data, "\r\n\r\n");
				if (body)
				{
					body += 4;
					printf("AI says: %s\r\n", body);
				}
				else
				{
					printf("could not parse AI response\r\n");
					HAL_UART_Transmit(&huart2, uart6_data, size6, HAL_MAX_DELAY);
				}
			}
			else
			{
				HAL_UART_Transmit(&huart2, uart6_data, size6, HAL_MAX_DELAY);
			}
		}
	}
}

void menu_task_handler(void *parameter)
{
	char* msg_menu = "=================\r\n"
			        "     MAIN MENU    \r\n"
			        "==================\r\n"
			        "AT Commands setup-->0\r\n"
			        "    pomodoro view-->1\r\n"
			        "Enter your choice--> ";
	while(1)
	{
		xQueueSend(q_print,&msg_menu,portMAX_DELAY);
		xTaskNotifyWait(0,0,0,portMAX_DELAY);

	}
}
void print_task_handler(void *parameter)
{
	char* print_data;
	while(1)
	{
		xQueueReceive(q_print, &print_data,portMAX_DELAY);
		xSemaphoreTake(uart2_mutex, portMAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t *)print_data,strlen(print_data),HAL_MAX_DELAY);
		xSemaphoreGive(uart2_mutex);
	}
}

void AT_task_handler(void *parameter)
{
	char *AT_msg = "============================\r\n"
			       "               AT Task      \r\n"
				   "=============================\r\n"
			       "view working mode          -->0\r\n"
			       "change working mode        -->1\r\n"
			       "set ssid and password      -->2\r\n"
			       "view current data and timer-->3\r\n"
			       "Enter new AI mode          -->4\r\n"
			       "Enter your choice          --> ";
	while(1)
	{
		xTaskNotifyWait(0,0,NULL,portMAX_DELAY);
		xQueueSend(q_print,&AT_msg,portMAX_DELAY);

	}
}
void pomodoro_view_handler(void *parameter)
{
	char *pomo_msg = "================\r\n"
			       "     Pomo Task    \r\n"
				   "================\r\n";
	while(1)
	{
		xTaskNotifyWait(0,0,NULL,portMAX_DELAY);
		xQueueSend(q_print,&pomo_msg,portMAX_DELAY);
		while (currState == sPomodorView)   // stay here, updating, until user exits
		{
			printf("time elapsed : %d:%d\r\n", minute, seconds);
			vTaskDelay(pdMS_TO_TICKS(1000));   // update once per second
		}


	}
}


//interupts for pomodoro
void ldr_interupt_handler()
{
	uint32_t now = HAL_GetTick();
		//software debouncing
	if((now - lastSeen)>DEBOUNCE_MS)
	{
		//printf("Hii interupt have been triggered\r\n");
		vTaskNotifyGiveFromISR(pomodoro_handle,NULL);

	}
	lastSeen = now;



}
void button_press_handler()
{
	uint32_t now = HAL_GetTick();
	//software debouncing
	if((now - lastSeen)>DEBOUNCE_MS)
	{
		//printf("Button is pressed\r\n");
		button_state = 1;
	}
	lastSeen = now;
	if(count == 1)
	{
		count = 0;
		pomoState = sPlay;
		HAL_TIM_Base_Start_IT(&htim10);
		xTaskResumeFromISR(pomodoro_handle);
		//vTaskNotifyGiveFromISR(pomodoro_handle,NULL);
	}



}

void pomodoro_handler_func(void *parameter)
{
	int lap=0;

	xTaskNotifyWait(0,0,NULL,portMAX_DELAY);
	HAL_TIM_Base_Start_IT(&htim10);
	while(1)
	{

		if(button_state == 1)
		{
			switch(pomoState)
			{
			case sPause:
			{
				count++;
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_SET);
				HAL_Delay(1000);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_RESET);
				pomoState = sPlay;
				button_state = 0;
				//printf("we are in paused state\r\n");
				temp_second = seconds;
				temp_minute = minute;
				HAL_TIM_Base_Stop_IT(&htim10);
				vTaskSuspend(pomodoro_handle);
				break;
			}
			case sPlay:
			{
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_SET);
				HAL_Delay(1000);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_RESET);
				pomoState = sPause;
				button_state = 0;
				//printf("we are in play state\r\n");
				break;

			}




			}

		}
		if ((minute % 25 == 0) && (minute != 0) && (seconds == 0))
		{
			lap++;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_SET);
			HAL_Delay(1000);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8,GPIO_PIN_RESET);
			if(count == 4) printf("Take a 15 minutes break now\r\n");
			else printf("Huraah you have successfully completed %d session\r\n",lap);
		}
		vTaskDelay(pdMS_TO_TICKS(1000));


		}



}
int _write(int file, char *ptr, int len)
{
	xSemaphoreTake(uart2_mutex,portMAX_DELAY);
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    xSemaphoreGive(uart2_mutex);
    return len;
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM9 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM9)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
