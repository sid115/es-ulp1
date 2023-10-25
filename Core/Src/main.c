/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include <stdbool.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef uint8_t crc;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MY_ADDRESS 3 /* board number */

#define MMCP_MASTER_ADDRESS 0
#define MMCP_VERSION 5

#define L7_PDU_size 9
#define L7_SDU_size 8
#define L7_PCI_size 1

#define L3_PDU_size 13
#define L3_SDU_size 9
#define L3_PCI_size 4

#define L2_PDU_size 14
#define L2_SDU_size 13
#define L2_PCI_size 1

#define L1_PDU_size 16
#define L1_SDU_size 14
#define L1_PCI_size 2

#define SOF 0
#define EOF 0

#define DEBOUNCE_INTERVAL 10 /* button debounce time in milliseconds */

#define CRC_POLYNOMIAL 0x9b
#define CRC_WIDTH  (8 * sizeof(crc))
#define CRC_TOPBIT (1 << (CRC_WIDTH - 1))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* GLOBALS */
bool dataReceived = false;
bool dataTransmitted = false;

uint8_t rxBuffer[L1_PDU_size] = { 0 };
uint8_t L1_PDU[L1_PDU_size] = { 0 };

uint8_t cnt = 0; /* button press counter */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_GPIO_EXTI_Callback ( uint16_t GPIO_Pin );

void L1_send(uint8_t L1_SDU[]);
void L1_receive(uint8_t L1_PDU[]);
void L2_send(uint8_t L2_SDU[]);
void L2_receive(uint8_t L2_PDU[]);
void L3_send(uint8_t L3_SDU[]);
void L3_receive(uint8_t L3_PDU[]);
void L7_send(uint8_t ApNr, uint8_t L7_SDU[]);
void L7_receive(uint8_t L7_PDU[]);

crc crcSlow(uint8_t const message[], int nBytes);
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
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_UART_Receive_IT(&huart2, rxBuffer, L1_PDU_size);

    if (dataReceived) {
      L1_receive(L1_PDU);

      dataReceived = false;

      while (!dataTransmitted) {} /* wait for response */
      dataTransmitted = false;
	}
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  huart1.Init.BaudRate = 115200;
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  memcpy(L1_PDU, rxBuffer, L1_PDU_size);

  dataReceived = true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  dataTransmitted = true;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static unsigned long millis = 0; /* elapsed time in milliseconds */
  static unsigned long lastPress = 0; /* time since last button press */

  millis = HAL_GetTick();
  
  if (GPIO_Pin == B1_Pin && (millis - lastPress) > DEBOUNCE_INTERVAL) {
    cnt++;
    lastPress = millis;
  }
}

void L1_send(uint8_t L1_SDU[])
{
  L1_PDU[0] = SOF;
  memcpy(&L1_PDU[1], L1_SDU, L1_SDU_size);
  L1_PDU[L1_PDU_size - 1] = EOF;
  
  HAL_UART_Transmit_IT(&huart2, L1_PDU, L1_PDU_size);
}

void L1_receive(uint8_t L1_PDU[])
{
  uint8_t L1_SDU[L1_SDU_size] = { 0 };

  memcpy(L1_SDU, &L1_PDU[1], L1_SDU_size);

  L2_receive(L1_SDU);
}

void L2_send(uint8_t L2_SDU[])
{
  uint8_t L2_PDU[L2_PDU_size] = { 0 };
  
  memcpy(L2_PDU, L2_SDU, L2_SDU_size);
  L2_PDU[L2_SDU_size] = crcSlow(L2_SDU, L2_SDU_size); /* checksum */
  
  L1_send(L2_PDU);
}

void L2_receive(uint8_t L2_PDU[])
{
  uint8_t L2_SDU[L2_SDU_size] = { 0 };
  uint8_t checksum = L2_PDU[L2_SDU_size];
  
  memcpy(L2_SDU, L2_PDU, L2_SDU_size);
  
  if (checksum == crcSlow(L2_SDU, L2_SDU_size)) {
    L3_receive(L2_SDU);
  } else {
    dataTransmitted = true; /* error */
  }
}

void L3_send(uint8_t L3_SDU[])
{
  uint8_t L3_PDU[L3_PDU_size] = { 0 };
  
  L3_PDU[0] = MMCP_MASTER_ADDRESS; /* to */
  L3_PDU[1] = MY_ADDRESS; /* from */
  L3_PDU[2] = MMCP_VERSION; /* version */

  memcpy(&L3_PDU[L3_PCI_size], L3_SDU, L3_SDU_size);
  
  L2_send(L3_PDU);
}

void L3_receive(uint8_t L3_PDU[])
{
  uint8_t L3_PCI[L3_PCI_size] = { 0 };
  uint8_t L3_SDU[L3_SDU_size] = { 0 };

  memcpy(L3_PCI, L3_PDU, L3_PCI_size);
  memcpy(L3_SDU, &L3_PDU[L3_PCI_size], L3_SDU_size);

  uint8_t to = L3_PCI[0];
  uint8_t from = L3_PCI[1];
  uint8_t version = L3_PCI[2];

  if (to != MMCP_MASTER_ADDRESS && from == MMCP_MASTER_ADDRESS && version == MMCP_VERSION) {
    if (L3_PDU[0] == MY_ADDRESS) {
      L7_receive(L3_SDU);
    } else {
      L3_PDU[3]++; /* cnt++ */
      L2_send(L3_PDU);
    }
  } else {
    dataTransmitted = true; /* error */
  }
}

void L7_send(uint8_t ApNr, uint8_t L7_SDU[])
{
  uint8_t L7_PDU[L7_PDU_size] = { 0 };

  L7_PDU[0] = ApNr;
  memcpy(&L7_PDU[1], L7_SDU, L7_SDU_size);

  L3_send(L7_PDU);
}

void L7_receive(uint8_t L7_PDU[])
{
  uint8_t L7_SDU[L7_SDU_size] = { 0 };
  uint8_t ApNr = L7_PDU[0];

  memcpy(L7_SDU, &L7_PDU[1], L7_SDU_size);

  switch (ApNr) {
    case 100: /* set LED */
      if (L7_SDU[7] != 0) { /* set LED on */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
      } else { /* set LED off */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
      }
      break;

    case 101: /* read number of keystrokes */
      L7_SDU[7] = cnt;
      cnt = 0;
      break;

    case 102: /* read out and return UID */
      uint32_t uid_part1 = HAL_GetUIDw0();
      uint32_t uid_part2 = HAL_GetUIDw1();

      memcpy(L7_SDU, &uid_part1, sizeof(uid_part1));
      memcpy(&L7_SDU[4], &uid_part2, sizeof(uid_part2));
      break;

    case 103: /* read out and return UID */
      uint32_t uid_part3 = HAL_GetUIDw2();

      memcpy(L7_SDU, &uid_part3, sizeof(uid_part3));
      break;

    default:
      dataTransmitted = true; /* error */
      break;
  }

  L7_send(ApNr, L7_SDU);
}

crc crcSlow(uint8_t const message[], int nBytes) {
  crc remainder = 0;

  for (int byte = 0; byte < nBytes; ++byte) {
    remainder ^= (message[byte] << (CRC_WIDTH - 8));
    for (uint8_t bit = 8; bit > 0; --bit){
      if (remainder & CRC_TOPBIT) {
        remainder = (remainder << 1) ^ CRC_POLYNOMIAL;
      } else {
        remainder = (remainder << 1);
      }
    }
  }

  return remainder;
}
/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
