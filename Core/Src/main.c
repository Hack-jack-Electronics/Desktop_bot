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
#define RX_BUFF_SIZE      2048
#define RING_BUFF_SIZE    2048

//I2S2 peripheral
#define I2S_SLOTS_TOTAL   512U
#define I2S_HALFWORDS     (I2S_SLOTS_TOTAL * 2U)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_rx;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_rx;

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

//I2S2 peripheral
static uint16_t i2sBuffer[I2S_HALFWORDS];
volatile uint8_t bufHalfReady = 0;
volatile uint8_t bufFullReady = 0;

/* === Stage 2: Audio streaming === */
#define SAMPLES_PER_HALF  128          // 128 left-channel samples per I2S half
#define BYTES_PER_HALF    (SAMPLES_PER_HALF * sizeof(int16_t))

static int16_t audioBufA[SAMPLES_PER_HALF];
static int16_t audioBufB[SAMPLES_PER_HALF];
static volatile uint8_t fillIndex = 0;      // 0 = fill A, 1 = fill B
static volatile uint8_t sendReady = 0;      // 1 = previous buffer has real data
static volatile uint8_t isRecording = 0;    // 1 = button pressed

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2S2_Init(void);
/* USER CODE BEGIN PFP */
void ProcessAudioBlock(uint16_t *data, uint16_t numHalfwords);
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
  MX_USART1_UART_Init();
  MX_I2S2_Init();
  /* USER CODE BEGIN 2 */
  // Baud is still 115200 here
  HAL_I2S_Receive_DMA(&hi2s2, i2sBuffer, I2S_SLOTS_TOTAL);

  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&huart1, esp32_rx_buffer, RX_BUFF_SIZE);


  // === STEP 1: Set WiFi mode (Station) ===
  char cmd[128];
  int len = snprintf(cmd, sizeof(cmd), "AT+CWMODE=1\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)cmd, len, HAL_MAX_DELAY);
  HAL_Delay(1000);

  // === STEP 2: Connect to WiFi ===
  len = snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"reenanup_2.4G\",\"15772424\"\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)cmd, len, HAL_MAX_DELAY);
  HAL_Delay(1000);

  // === STEP 3: Check IP ===
  len = snprintf(cmd, sizeof(cmd), "AT+CIFSR\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)cmd, len, HAL_MAX_DELAY);
  HAL_Delay(1000);

  len = snprintf(cmd, sizeof(cmd), "AT+CWLAP\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)cmd, len, HAL_MAX_DELAY);
  HAL_Delay(1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
  		if (bufHalfReady) {
  			bufHalfReady = 0;
  			ProcessAudioBlock(&i2sBuffer[0], I2S_HALFWORDS / 2U);
  		}
  		if (bufFullReady) {
  			bufFullReady = 0;
  			ProcessAudioBlock(&i2sBuffer[I2S_HALFWORDS / 2U],
  					I2S_HALFWORDS / 2U);
  		}
  		/* USER CODE END WHILE */
//  		while (esp_ring_head != esp_ring_tail) {
//  			uint8_t byte = esp_ring_buffer[esp_ring_tail];
//  			HAL_UART_Transmit(&huart2, &byte, 1, HAL_MAX_DELAY);
//  			esp_ring_tail = (esp_ring_tail + 1) % RING_BUFF_SIZE;
//  		}

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
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_16K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 921600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.Init.BaudRate = 921600;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

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

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void USART2_IRQHandler(void)
{
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
  HAL_UART_IRQHandler(&huart2);
}

void USART1_IRQHandler(void)
{
  if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_IDLE))
  {
      __HAL_UART_CLEAR_IDLEFLAG(&huart1);
      uint16_t dma_esp_counter = __HAL_DMA_GET_COUNTER(huart1.hdmarx);
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
  HAL_UART_IRQHandler(&huart1);
}

void ring_buffer_write(uint8_t *data, uint16_t len)
{
    for (int i = 0; i < len; i++)
    {
        ring_buffer[ring_head] = data[i];
        ring_head = (ring_head + 1) % RING_BUFF_SIZE;
    }
}

void esp_ring_buffer_write(uint8_t *data, uint16_t len)
{
    for (int i = 0; i < len; i++)
    {
        esp_ring_buffer[esp_ring_head] = data[i];
        esp_ring_head = (esp_ring_head + 1) % RING_BUFF_SIZE;
    }
}

void ProcessAudioBlock(uint16_t *data, uint16_t numHalfwords)
{
    /* Choose which buffer to FILL, and which to SEND */
    int16_t *fill = (fillIndex == 0) ? audioBufA : audioBufB;
    int16_t *send = (fillIndex == 0) ? audioBufB : audioBufA;

    /* Extract left channel, 24-bit -> 16-bit */
    uint16_t pairCount = numHalfwords / 4U;
    for (uint16_t i = 0; i < pairCount; i++)
    {
        uint16_t msb = data[i * 4U + 0U];
        uint16_t lsb = data[i * 4U + 1U];
        uint32_t raw = ((uint32_t)msb << 16) | lsb;
        int32_t sample24 = ((int32_t)raw) >> 8;
        fill[i] = (int16_t)(sample24 >> 8);   // Keep top 16 bits
    }

    /* If recording and we have a previous buffer ready, send it raw */
    if (isRecording && sendReady)
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)send, BYTES_PER_HALF, HAL_MAX_DELAY);
    }

    sendReady = 1;       // Next callback, this buffer becomes "sendable"
    fillIndex ^= 1;      // Swap fill target
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) bufHalfReady = 1;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) bufFullReady = 1;
    //HAL_I2S_Receive_DMA(&hi2s2, i2sBuffer, I2S_SLOTS_TOTAL);
}
/* Button press/release */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != GPIO_PIN_0) return;

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    {
        isRecording = 1;
        sendReady = 0;
        fillIndex = 0;
        HAL_UART_Transmit(&huart2, (uint8_t*)"REC START\r\n", 11, 100);
    }
    else
    {
        isRecording = 0;
        HAL_UART_Transmit(&huart2, (uint8_t*)"REC STOP\r\n", 10, 100);
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
