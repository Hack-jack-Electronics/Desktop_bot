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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

/* === FINAL: ESP32 State Machine Timing === */
#define ESP_CMD_TIMEOUT_MS       10000
#define ESP_WIFI_TIMEOUT_MS      20000
#define ESP_TCP_TIMEOUT_MS       15000
#define ESP_RETRY_MAX            5
#define ESP_TCP_RETRY_DELAY_MS   5000

/* === FINAL: TCP Audio Streaming === */
#define TCP_CHUNK_SIZE           2048   // audio payload per frame
#define TCP_SEND_WAIT_MS         15
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;
I2S_HandleTypeDef hi2s2ext;
DMA_HandleTypeDef hdma_i2s2_ext_tx;
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
#define SAMPLES_PER_HALF  128
#define BYTES_PER_HALF    (SAMPLES_PER_HALF * sizeof(int16_t))

static int16_t audioBufA[SAMPLES_PER_HALF];
static int16_t audioBufB[SAMPLES_PER_HALF];
static volatile uint8_t fillIndex = 0;
static volatile uint8_t sendReady = 0;
static volatile uint8_t isRecording = 0;

/* === FINAL: Button debounce & safe flags === */
static volatile uint32_t lastButtonMs = 0;
static volatile uint8_t btnPressFlag = 0;
static volatile uint8_t btnReleaseFlag = 0;
static volatile uint8_t btnLastState = 0;
#define BTN_DEBOUNCE_MS 300

/* === FINAL: TCP Audio Buffers (length-prefixed) === */
// Each chunk on wire: [2-byte len, little-endian][2048 bytes audio]
#define TCP_WIRE_SIZE      (2 + TCP_CHUNK_SIZE)
static uint8_t tcpAccumA[TCP_CHUNK_SIZE];
static uint8_t tcpAccumB[TCP_CHUNK_SIZE];
static uint8_t *tcpAccumFill = tcpAccumA;
static uint8_t *tcpAccumSendPtr = NULL;
static uint16_t tcpAccumIdx = 0;
static uint16_t tcpSendLen = 0;  // total wire bytes (prefix + audio)
static volatile uint8_t tcpSendPending = 0;

typedef enum {
    TCP_IDLE,
    TCP_CMD,
    TCP_WAIT,
    TCP_DATA,
    TCP_WAIT_OK
} TCP_SendState_t;
static TCP_SendState_t tcpSendState = TCP_IDLE;
static uint32_t tcpTimer = 0;

/* === FINAL: ESP32 State Machine === */
typedef enum {
    ESP_INIT,
    ESP_WAIT_AT,
    ESP_SET_MODE,
    ESP_WAIT_MODE,
    ESP_SET_MUX,
    ESP_WAIT_MUX,
    ESP_CONNECT_WIFI,
    ESP_WAIT_WIFI,
    ESP_VERIFY_IP,
    ESP_WAIT_IP,
    ESP_WIFI_READY,
    ESP_TCP_START,
    ESP_WAIT_TCP_CONNECT,
    ESP_TCP_CLOSE_FOR_RETRY,
    ESP_WAIT_CLOSE,
    ESP_TCP_TEST_SEND,
    ESP_WAIT_PROMPT,
    ESP_WAIT_SEND_OK,
    ESP_AUDIO_READY,
    ESP_ERROR
} ESP_State_t;

static ESP_State_t espState = ESP_INIT;
static uint32_t espTimer = 0;
static uint32_t espBootDelay = 0;
static uint8_t atRetries = 0;

static uint8_t respOK = 0;
static uint8_t respError = 0;
static uint8_t respConnect = 0;
static uint8_t respAlreadyConnected = 0;
static uint8_t respSendOK = 0;
static uint8_t respClosed = 0;
static uint32_t tcpRetryDelay = 0;

static char espStaIp[16] = {0};

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
void ESP32_ProcessResponses(void);
void ESP32_StateMachine(void);
void TCP_AudioSendStateMachine(void);
void ring_buffer_write(uint8_t *data, uint16_t len);
void esp_ring_buffer_write(uint8_t *data, uint16_t len);
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
  HAL_UART_Receive_DMA(&huart1, esp32_rx_buffer, RX_BUFF_SIZE);
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  static uint16_t txDummyBuffer[I2S_HALFWORDS] = {0};
  // CORRECT — 4 arguments, only the main handle
  HAL_I2SEx_TransmitReceive_DMA(&hi2s2, txDummyBuffer, i2sBuffer, I2S_SLOTS_TOTAL);
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  /* USER CODE BEGIN 3 */
	        ESP32_ProcessResponses();
	        ESP32_StateMachine();

	        if (btnPressFlag)
	        {
	            btnPressFlag = 0;
	            if (espState == ESP_AUDIO_READY) {
	                HAL_UART_Transmit(&huart2, (uint8_t*)"REC START\r\n", 11, 100);
	            } else {
	                HAL_UART_Transmit(&huart2, (uint8_t*)"BTN: not ready\r\n", 16, 100);
	            }
	        }
	        if (btnReleaseFlag)
	        {
	            btnReleaseFlag = 0;
	            HAL_UART_Transmit(&huart2, (uint8_t*)"REC STOP\r\n", 10, 100);
	        }

	        if (tcpSendPending && tcpSendState == TCP_IDLE && espState == ESP_AUDIO_READY)
	        {
	            tcpSendPending = 0;
	            tcpSendState = TCP_CMD;
	        }
	        TCP_AudioSendStateMachine();

	        if (bufHalfReady) {
	            bufHalfReady = 0;
	            ProcessAudioBlock(&i2sBuffer[0], I2S_HALFWORDS / 2U);
	        }
	        if (bufFullReady) {
	            bufFullReady = 0;
	            ProcessAudioBlock(&i2sBuffer[I2S_HALFWORDS / 2U], I2S_HALFWORDS / 2U);
	        }
	  /* USER CODE END 3 */

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
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */
  /* USER CODE BEGIN I2S2_Init 2 */
  hi2s2ext.Instance = I2S2ext;
  hi2s2ext.Init.Mode = I2S_MODE_SLAVE_TX;
  hi2s2ext.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2ext.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s2ext.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s2ext.Init.AudioFreq = I2S_AUDIOFREQ_16K;
  hi2s2ext.Init.CPOL = I2S_CPOL_LOW;
  hi2s2ext.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2ext.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s2ext) != HAL_OK)
  {
      Error_Handler();
  }
  /* USER CODE END I2S2_Init 2 */

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
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
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
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;  // Was GPIO_MODE_IT_RISING
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* === FINAL: ESP32 Helper Functions === */

int ESP32_ReadLine(char *line, uint16_t maxLen)
{
    uint16_t i = 0;
    uint16_t tempTail = esp_ring_tail;

    while (tempTail != esp_ring_head && i < maxLen - 1)
    {
        line[i] = esp_ring_buffer[tempTail];
        tempTail = (tempTail + 1) % RING_BUFF_SIZE;
        i++;

        if (i >= 2 && line[i - 2] == '\r' && line[i - 1] == '\n')
        {
            line[i] = '\0';
            esp_ring_tail = tempTail;
            return i;
        }
    }
    return 0;
}

uint8_t ESP32_ParseCIFSR_IP(const char *line, char *ipOut, uint8_t maxLen)
{
    const char *p = strstr(line, "STAIP,");
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p++;
    uint8_t i = 0;
    while (*p && *p != '"' && i < maxLen - 1)
    {
        ipOut[i++] = *p++;
    }
    ipOut[i] = '\0';
    return (i > 0 && strcmp(ipOut, "0.0.0.0") != 0);
}

void ESP32_ProcessResponses(void)
{
    char line[128];
    while (ESP32_ReadLine(line, sizeof(line)))
    {
        if (strstr(line, "SEND OK"))
            respSendOK = 1;
        if (strstr(line, "OK"))
            respOK = 1;
        if (strstr(line, "ERROR") || strstr(line, "FAIL") || strstr(line, "busy"))
            respError = 1;
        if (strstr(line, "CONNECT") && !strstr(line, "FAIL"))
            respConnect = 1;
        if (strstr(line, "ALREADY CONNECTED"))
            respAlreadyConnected = 1;
        if (strstr(line, "CLOSED"))
            respClosed = 1;
        if (ESP32_ParseCIFSR_IP(line, espStaIp, sizeof(espStaIp))) { /* IP parsed */ }
    }
}

void ESP32_ClearFlags(void)
{
    respOK = 0;
    respError = 0;
    respConnect = 0;
    respAlreadyConnected = 0;
    respSendOK = 0;
    respClosed = 0;
}

void ESP32_SendCommand(const char *cmd)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), HAL_MAX_DELAY);
    ESP32_ClearFlags();
    espTimer = HAL_GetTick();
}

void ESP32_StateMachine(void)
{
    switch (espState)
    {
        case ESP_INIT:
            if (espBootDelay == 0) espBootDelay = HAL_GetTick();
            if ((HAL_GetTick() - espBootDelay) > 2000)
            {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Stage 1: Starting...\r\n", 28, 100);
                ESP32_SendCommand("AT\r\n");
                espState = ESP_WAIT_AT;
            }
            break;

        case ESP_WAIT_AT:
            if (respOK) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] AT OK\r\n", 13, 100);
                ESP32_SendCommand("AT+CWMODE=1\r\n");
                espState = ESP_WAIT_MODE; atRetries = 0;
            } else if (respError || (HAL_GetTick() - espTimer) > ESP_CMD_TIMEOUT_MS) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] AT fail, retry\r\n", 22, 100);
                espBootDelay = 0; espState = ESP_INIT; atRetries++;
                if (atRetries > ESP_RETRY_MAX) espState = ESP_ERROR;
            }
            break;

        case ESP_SET_MODE: espState = ESP_WAIT_MODE; break;

        case ESP_WAIT_MODE:
            if (respOK) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] STA mode OK\r\n", 19, 100);
                ESP32_SendCommand("AT+CIPMUX=0\r\n");
                espState = ESP_WAIT_MUX;
            } else if (respError || (HAL_GetTick() - espTimer) > ESP_CMD_TIMEOUT_MS) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Mode fail\r\n", 17, 100);
                espState = ESP_INIT;
            }
            break;

        case ESP_SET_MUX: espState = ESP_WAIT_MUX; break;

        case ESP_WAIT_MUX:
            if (respOK) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] MUX=0 OK\r\n", 16, 100);
                char cmd[128];
                snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"reenanup_2.4G\",\"15772424\"\r\n");
                ESP32_SendCommand(cmd);
                espState = ESP_WAIT_WIFI;
            } else if (respError || (HAL_GetTick() - espTimer) > ESP_CMD_TIMEOUT_MS) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] MUX fail\r\n", 16, 100);
                espState = ESP_INIT;
            }
            break;

        case ESP_CONNECT_WIFI: espState = ESP_WAIT_WIFI; break;

        case ESP_WAIT_WIFI:
            if (respOK && !respError) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] WiFi cmd OK, checking IP...\r\n", 35, 100);
                memset(espStaIp, 0, sizeof(espStaIp));
                ESP32_SendCommand("AT+CIFSR\r\n");
                espState = ESP_WAIT_IP;
            } else if (respError || (HAL_GetTick() - espTimer) > ESP_WIFI_TIMEOUT_MS) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] WiFi conn failed\r\n", 24, 100);
                espState = ESP_INIT;
            }
            break;

        case ESP_VERIFY_IP: espState = ESP_WAIT_IP; break;

        case ESP_WAIT_IP:
            if (respOK) {
                if (strlen(espStaIp) > 0) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "[ESP] IP=%s\r\n", espStaIp);
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
                    HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Stage 1 COMPLETE. WiFi Ready.\r\n", 37, 100);
                    espState = ESP_WIFI_READY; atRetries = 0;
                } else {
                    HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] IP=0.0.0.0, retry WiFi\r\n", 30, 100);
                    espState = ESP_INIT;
                }
            } else if (respError || (HAL_GetTick() - espTimer) > ESP_CMD_TIMEOUT_MS) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] IP check fail\r\n", 21, 100);
                espState = ESP_INIT;
            }
            break;

        case ESP_WIFI_READY:
        {
            static uint8_t announced = 0;
            if (!announced) { HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Ready for Stage 2 (TCP)\r\n", 31, 100); announced = 1; }
            static uint32_t t2 = 0;
            if (t2 == 0) t2 = HAL_GetTick();
            if ((HAL_GetTick() - t2) > 1000) { espState = ESP_TCP_START; t2 = 0; }
            break;
        }

        case ESP_TCP_START:
            if (tcpRetryDelay > 0) {
                if ((HAL_GetTick() - tcpRetryDelay) < ESP_TCP_RETRY_DELAY_MS) break;
                tcpRetryDelay = 0;
            }
            HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Stage 2: Connecting TCP...\r\n", 34, 100);
            /* === REPLACE IP WITH YOUR PC'S WIFI IP === */
            ESP32_SendCommand("AT+CIPSTART=\"TCP\",\"192.168.1.23\",12345\r\n");
            espState = ESP_WAIT_TCP_CONNECT;
            break;

        case ESP_WAIT_TCP_CONNECT:
            if (respConnect || respAlreadyConnected) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] TCP Connected\r\n", 21, 100);
                ESP32_ClearFlags();
                espState = ESP_TCP_TEST_SEND;
            } else if (respError || respClosed || (HAL_GetTick() - espTimer) > ESP_TCP_TIMEOUT_MS) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] TCP conn failed, closing...\r\n", 35, 100);
                ESP32_SendCommand("AT+CIPCLOSE\r\n");
                espState = ESP_TCP_CLOSE_FOR_RETRY;
            }
            break;

        case ESP_TCP_CLOSE_FOR_RETRY: espState = ESP_WAIT_CLOSE; break;

        case ESP_WAIT_CLOSE:
            if (respOK || respClosed || (HAL_GetTick() - espTimer) > 3000) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Retry TCP in 5s...\r\n", 26, 100);
                tcpRetryDelay = HAL_GetTick();
                espState = ESP_TCP_START;
            }
            break;

        case ESP_TCP_TEST_SEND:
            ESP32_SendCommand("AT+CIPSEND=5\r\n");
            espState = ESP_WAIT_PROMPT;
            break;

        case ESP_WAIT_PROMPT:
            if (respError) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] CIPSEND rejected\r\n", 24, 100);
                espState = ESP_TCP_START;
            } else if ((HAL_GetTick() - espTimer) > 300) {
                HAL_UART_Transmit(&huart1, (uint8_t*)"HELLO", 5, HAL_MAX_DELAY);
                espState = ESP_WAIT_SEND_OK;
                espTimer = HAL_GetTick();
            }
            break;

        case ESP_WAIT_SEND_OK:
            if (respSendOK || respOK) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] HELLO OK. Audio ready.\r\n", 30, 100);
                espState = ESP_AUDIO_READY;
            } else if (respError || (HAL_GetTick() - espTimer) > 5000) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Send timeout\r\n", 20, 100);
                espState = ESP_TCP_START;
            }
            break;

        case ESP_AUDIO_READY:
        {
            static uint8_t announced = 0;
            if (!announced) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Stage 4 ACTIVE. Audio -> TCP.\r\n", 37, 100);
                HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] Press button to stream.\r\n", 31, 100);
                announced = 1;
            }
            break;
        }

        case ESP_ERROR:
            HAL_UART_Transmit(&huart2, (uint8_t*)"[ESP] FATAL. Reset in 5s...\r\n", 29, 100);
            HAL_Delay(5000);
            NVIC_SystemReset();
            break;

        default: espState = ESP_INIT; break;
    }
}

/* === FINAL: TCP Audio Send State Machine === */
void TCP_AudioSendStateMachine(void)
{
    switch (tcpSendState)
    {
        case TCP_IDLE: break;

        case TCP_CMD:
        {
            respSendOK = 0; respOK = 0; respError = 0; respClosed = 0;
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", tcpSendLen);
            HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), HAL_MAX_DELAY);

            // --- DEBUG ---
            char dbg[48];
            snprintf(dbg, sizeof(dbg), "[TCP] CMD CIPSEND=%d\r\n", tcpSendLen);
            HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 50);
            // -------------

            tcpTimer = HAL_GetTick();
            tcpSendState = TCP_WAIT;
            break;
        }

        case TCP_WAIT:
            if ((HAL_GetTick() - tcpTimer) > TCP_SEND_WAIT_MS) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[TCP] WAIT->DATA\r\n", 18, 50);
                tcpSendState = TCP_DATA;
            }
            break;

        case TCP_DATA:
        {
            if (tcpAccumSendPtr != NULL && tcpSendLen > 0) {
                HAL_UART_Transmit(&huart1, tcpAccumSendPtr, tcpSendLen, HAL_MAX_DELAY);

                // --- DEBUG ---
                char dbg[48];
                snprintf(dbg, sizeof(dbg), "[TCP] SENT %d bytes\r\n", tcpSendLen);
                HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 50);
                // -------------
            }
            tcpTimer = HAL_GetTick();
            tcpSendState = TCP_WAIT_OK;
            break;
        }

        case TCP_WAIT_OK:
            if (respSendOK || respOK) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[TCP] SEND OK\r\n", 15, 50);
                tcpSendState = TCP_IDLE;
                tcpAccumSendPtr = NULL;
                tcpSendLen = 0;
            } else if (respError || respClosed || (HAL_GetTick() - tcpTimer) > 5000) {
                HAL_UART_Transmit(&huart2, (uint8_t*)"[TCP] SEND FAIL / TIMEOUT\r\n", 27, 50);
                tcpSendState = TCP_IDLE;
                tcpAccumSendPtr = NULL;
                tcpSendLen = 0;
                espState = ESP_TCP_START;
            }
            break;
    }
}

/* === Existing UART IRQHandlers (unchanged) === */
void USART2_IRQHandler(void)
{
  if(__HAL_UART_GET_FLAG(&huart2,UART_FLAG_IDLE))
  {
      __HAL_UART_CLEAR_IDLEFLAG(&huart2);
      uint16_t dma_counter = __HAL_DMA_GET_COUNTER(huart2.hdmarx);
      rx_curr_pos = RX_BUFF_SIZE - dma_counter;
      if(rx_curr_pos != rx_prev_pos)
      {
          if(rx_curr_pos > rx_prev_pos) {
              uint16_t len = rx_curr_pos - rx_prev_pos;
              ring_buffer_write(&rx_buffer[rx_prev_pos],len);
              rx_prev_pos = rx_curr_pos;
          } else {
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
          if (esp_rx_curr_pos > esp_rx_prev_pos) {
              uint16_t len = esp_rx_curr_pos - esp_rx_prev_pos;
              esp_ring_buffer_write(&esp32_rx_buffer[esp_rx_prev_pos], len);
              esp_rx_prev_pos = esp_rx_curr_pos;
          } else {
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
    for (int i = 0; i < len; i++) {
        ring_buffer[ring_head] = data[i];
        ring_head = (ring_head + 1) % RING_BUFF_SIZE;
    }
}

void esp_ring_buffer_write(uint8_t *data, uint16_t len)
{
    for (int i = 0; i < len; i++) {
        esp_ring_buffer[esp_ring_head] = data[i];
        esp_ring_head = (esp_ring_head + 1) % RING_BUFF_SIZE;
    }
}

/* === FINAL: Audio Processing -> TCP with length prefix === */
/* === FINAL: Audio Processing -> TCP with length prefix === */
void ProcessAudioBlock(uint16_t *data, uint16_t numHalfwords)
{
    int16_t *fill = (fillIndex == 0) ? audioBufA : audioBufB;
    int16_t *send = (fillIndex == 0) ? audioBufB : audioBufA;

    uint16_t pairCount = numHalfwords / 4U;
    for (uint16_t i = 0; i < pairCount; i++)
    {
        uint16_t msb = data[i * 4U + 0U];
        uint16_t lsb = data[i * 4U + 1U];
        uint32_t raw = ((uint32_t)msb << 16) | lsb;
        int32_t sample24 = ((int32_t)raw) >> 8;
        fill[i] = (int16_t)(sample24 >> 8);
    }

    if (isRecording && sendReady && espState == ESP_AUDIO_READY)
    {
        if (tcpAccumIdx + BYTES_PER_HALF <= TCP_CHUNK_SIZE)
        {
            // --- DEBUG: accumulation progress ---
            if (tcpAccumIdx == BYTES_PER_HALF || tcpAccumIdx % 512 == 0) {
                char dbg[48];
                snprintf(dbg, sizeof(dbg), "[AUD] Accum %u/%u\r\n", tcpAccumIdx, TCP_CHUNK_SIZE);
                HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 50);
            }
            // ------------------------------------
            memcpy(&tcpAccumFill[tcpAccumIdx], send, BYTES_PER_HALF);
            tcpAccumIdx += BYTES_PER_HALF;

            if (tcpAccumIdx >= TCP_CHUNK_SIZE && !tcpSendPending)
            {
                // Build wire frame: [2-byte len, little-endian][audio data]
                static uint8_t wireFrame[2 + TCP_CHUNK_SIZE];
                wireFrame[0] = (uint8_t)(TCP_CHUNK_SIZE & 0xFF);
                wireFrame[1] = (uint8_t)((TCP_CHUNK_SIZE >> 8) & 0xFF);
                memcpy(&wireFrame[2], tcpAccumFill, TCP_CHUNK_SIZE);

                tcpAccumSendPtr = wireFrame;
                tcpSendLen = 2 + TCP_CHUNK_SIZE;  // prefix + audio
                tcpAccumFill = (tcpAccumFill == tcpAccumA) ? tcpAccumB : tcpAccumA;
                tcpAccumIdx = 0;
                tcpSendPending = 1;
                // --- DEBUG: chunk ready ---
                {
                    char dbg[48];
                    snprintf(dbg, sizeof(dbg), "[AUD] CHUNK READY %u bytes -> TCP\r\n", tcpSendLen);
                    HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 50);
                }
                // --------------------------
            }
        }
    }

    sendReady = 1;
    fillIndex ^= 1;
}


void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) bufHalfReady = 1;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) bufFullReady = 1;
}

/* === FINAL: Button ISR === */
/* === FINAL: Button ISR === */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != GPIO_PIN_0) return;

    uint32_t now = HAL_GetTick();
    if ((now - lastButtonMs) < BTN_DEBOUNCE_MS) return;

    uint8_t pinState = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? 1 : 0;
    if (pinState == btnLastState) return;

    btnLastState = pinState;
    lastButtonMs = now;

    if (pinState == 1)
    {
        isRecording = 1;
        sendReady = 0;
        fillIndex = 0;
        tcpAccumIdx = 0;
        btnPressFlag = 1;
        HAL_UART_Transmit(&huart2, (uint8_t*)"[BTN] PRESS -> RECORDING\r\n", 26, 50);
    }
    else
    {
        isRecording = 0;
        btnReleaseFlag = 1;

        // --- DEBUG + FLUSH PARTIAL BUFFER ---
        char dbg[64];
        snprintf(dbg, sizeof(dbg), "[BTN] RELEASE, partial=%u bytes\r\n", tcpAccumIdx);
        HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 50);

        if (tcpAccumIdx > 0 && espState == ESP_AUDIO_READY && !tcpSendPending)
        {
            // Build final length-prefixed frame with actual size
            static uint8_t flushFrame[2 + TCP_CHUNK_SIZE];
            flushFrame[0] = (uint8_t)(tcpAccumIdx & 0xFF);
            flushFrame[1] = (uint8_t)((tcpAccumIdx >> 8) & 0xFF);
            memcpy(&flushFrame[2], tcpAccumFill, tcpAccumIdx);

            tcpAccumSendPtr = flushFrame;
            tcpSendLen = 2 + tcpAccumIdx;
            tcpSendPending = 1;

            HAL_UART_Transmit(&huart2, (uint8_t*)"[BTN] FLUSHED partial\r\n", 23, 50);
        }
        // ------------------------------------
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
