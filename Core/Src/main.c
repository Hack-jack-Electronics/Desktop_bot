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
#define RX_BUFF_SIZE 2048
#define RING_BUFF_SIZE 2048
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart6_rx;

/* USER CODE BEGIN PV */
uint8_t rx_buffer[RX_BUFF_SIZE];
uint8_t esp32_rx_buffer[RX_BUFF_SIZE];
uint8_t ring_buffer[RING_BUFF_SIZE];

uint16_t rx_curr_pos = 0;
uint16_t rx_prev_pos = 0;
uint16_t ring_head = 0;
uint16_t ring_tail = 0;
uint8_t esp_ring_buffer[RING_BUFF_SIZE];

uint16_t esp_rx_curr_pos = 0;
uint16_t esp_rx_prev_pos = 0;
uint16_t esp_ring_head = 0;
uint16_t esp_ring_tail = 0;

//char cmd[] = "AT+CWJAP=\"reenanup_2.4G\",\"15772424\"\r\n";
char cmd[] = "AT+CWLAP\r\n";

char http_request[256];
char body[] = "RIFF....WAVE....";  // We'll make a real WAV header later

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */

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
  /* USER CODE BEGIN 2 */
//  __HAL_UART_ENABLE_IT(&huart2,UART_IT_IDLE);
//  HAL_UART_Receive_DMA(&huart2,rx_buffer,RX_BUFF_SIZE);
  __HAL_UART_ENABLE_IT(&huart6,UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart6,esp32_rx_buffer,RX_BUFF_SIZE);

    /* USER CODE BEGIN 2 */
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart6, esp32_rx_buffer, RX_BUFF_SIZE);

    char cmd[128];
    int len;

    // === STEP 1: Set WiFi mode (Station) ===
    len = sprintf(cmd, "AT+CWMODE=1\r\n");
    HAL_UART_Transmit(&huart6, (uint8_t*)cmd, len, HAL_MAX_DELAY);
    HAL_Delay(1000);

    // === STEP 2: Connect to YOUR WiFi ===
    // REPLACE "reenanup_2.4G" and "15772424" with your real SSID and password
    len = sprintf(cmd, "AT+CWJAP=\"reenanup_2.4G\",\"15772424\"\r\n");
    HAL_UART_Transmit(&huart6, (uint8_t*)cmd, len, HAL_MAX_DELAY);
    HAL_Delay(8000);  // WiFi takes 3-8 seconds!

    // === STEP 3: Check if we got IP ===
    len = sprintf(cmd, "AT+CIFSR\r\n");
    HAL_UART_Transmit(&huart6, (uint8_t*)cmd, len, HAL_MAX_DELAY);
    HAL_Delay(2000);

    // === STEP 4: NOW open TCP ===
    len = sprintf(cmd, "AT+CIPSTART=\"TCP\",\"192.168.1.23\",5000\r\n");
    HAL_UART_Transmit(&huart6, (uint8_t*)cmd, len, HAL_MAX_DELAY);
    HAL_Delay(4000);

    // === STEP 5: Build HTTP POST ===
    char http_body[] = "RIFF0000WAVE000000000000000000000000000000000000000000000";  // Passes RIFF/WAVE check
    int body_len;
    body_len = strlen(http_body);

    char http_request[512];
    int req_len;
    req_len = sprintf(http_request,
        "POST /transcribe HTTP/1.1\r\n"
        "Host: 192.168.1.23:5000\r\n"
        "Content-Type: audio/wav\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s", body_len, http_body);

    // === STEP 6: Tell ESP32 how many bytes to expect ===
    int cipsend_len;
    cipsend_len = sprintf(cmd, "AT+CIPSEND=%d\r\n", req_len);
    HAL_UART_Transmit(&huart6, (uint8_t*)cmd, cipsend_len, HAL_MAX_DELAY);
    HAL_Delay(1000);  // Wait for ">" prompt

    // === STEP 7: Send the actual HTTP request ===
    HAL_UART_Transmit(&huart6, (uint8_t*)http_request, req_len, HAL_MAX_DELAY);
    HAL_Delay(3000);  // Wait for server reply

    /* USER CODE END 2 */


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  while(esp_ring_head != esp_ring_tail)
	  {
		  uint8_t byte = esp_ring_buffer[esp_ring_tail];
		  HAL_UART_Transmit(&huart2,&byte,1,HAL_MAX_DELAY);
		  esp_ring_tail = (esp_ring_tail + 1) % RING_BUFF_SIZE;


	  }

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
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// when mera IDLE interupt triggers it will come here
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
	if(__HAL_UART_GET_FLAG(&huart2,UART_FLAG_IDLE))
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart2);

		uint16_t dma_counter = __HAL_DMA_GET_COUNTER(huart2.hdmarx);
		rx_curr_pos = RX_BUFF_SIZE - dma_counter;

		if(rx_curr_pos != rx_prev_pos)
		{
			if(rx_curr_pos > rx_prev_pos)
			{
				uint16_t len = rx_curr_pos - rx_prev_pos;
				ring_buffer_write(&rx_buffer[rx_prev_pos],len);
				rx_prev_pos = rx_curr_pos;

			}
			else
			{
				uint16_t len1 = RX_BUFF_SIZE - rx_prev_pos;
				ring_buffer_write(&rx_buffer[rx_prev_pos], len1);
				uint16_t len2 = rx_curr_pos;
				ring_buffer_write(&rx_buffer[0], len2);
				rx_prev_pos = rx_curr_pos;
			}
		}
	}

  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

void USART6_IRQHandler(void)
{
  /* USER CODE BEGIN USART6_IRQn 0 */
	if(__HAL_UART_GET_FLAG(&huart6,UART_FLAG_IDLE))
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart6);

		uint16_t dma_esp_counter = __HAL_DMA_GET_COUNTER(huart6.hdmarx);
		esp_rx_curr_pos = RX_BUFF_SIZE - dma_esp_counter;

		if (esp_rx_curr_pos != esp_rx_prev_pos)
		{
			if (esp_rx_curr_pos > esp_rx_prev_pos)
			{
				uint16_t len = esp_rx_curr_pos - esp_rx_prev_pos;
				esp_ring_buffer_write(&esp32_rx_buffer[esp_rx_prev_pos], len);
				esp_rx_prev_pos = esp_rx_curr_pos;
			}
			else
			{
				uint16_t len1 = RX_BUFF_SIZE - esp_rx_prev_pos;
				esp_ring_buffer_write(&esp32_rx_buffer[esp_rx_prev_pos], len1);
				uint16_t len2 = esp_rx_curr_pos;
				esp_ring_buffer_write(&esp32_rx_buffer[0], len2);
				esp_rx_prev_pos = esp_rx_curr_pos;
			}

		}

	}

  /* USER CODE END USART6_IRQn 0 */
  HAL_UART_IRQHandler(&huart6);
  /* USER CODE BEGIN USART6_IRQn 1 */

  /* USER CODE END USART6_IRQn 1 */
}
void ring_buffer_write(uint8_t *data, uint16_t len)
{
	for (int i = 0 ; i < len ;i++)
	{
		ring_buffer[ring_head] = data[i];
		ring_head = (ring_head + 1 )% RING_BUFF_SIZE;
	}
}
void esp_ring_buffer_write(uint8_t *data, uint16_t len)
{
	for (int i = 0 ; i < len ;i++)
	{
		esp_ring_buffer[esp_ring_head] = data[i];
		esp_ring_head = (esp_ring_head + 1 )% RING_BUFF_SIZE;
	}
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
