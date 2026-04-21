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
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DS18B20 Temperature Sensor - STM32 to Raspberry Pi
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint8_t S1_ID[8] = {0x28, 0x86, 0x84, 0x37, 0x00, 0x00, 0x00, 0x5C};
uint8_t S2_ID[8] = {0x28, 0xCC, 0x48, 0x38, 0x00, 0x00, 0x00, 0x7F};
uint8_t S3_ID[8] = {0x28, 0xE7, 0xB4, 0x36, 0x00, 0x00, 0x00, 0xA3};

float T1, T2, T3;
uint8_t presence = 0;
/* USER CODE END PV */
/* USER CODE BEGIN PFP */
void delay_us (uint16_t us);
uint8_t DS18B20_Start (void);
void DS18B20_Write (uint8_t data);
uint8_t DS18B20_Read (void);
/* USER CODE END PFP */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN 0 */
void DS18B20_Config9Bit(void) {
    if (DS18B20_Start()) {
        DS18B20_Write(0xCC); // Tüm sensörlere söyle
        DS18B20_Write(0x4E); // Yazma komutu
        DS18B20_Write(0x00); // TH (önemsiz)
        DS18B20_Write(0x00); // TL (önemsiz)
        DS18B20_Write(0x1F); // 0x1F = 9-bit çözünürlük
    }
}
void delay_us (uint16_t us) {
	__HAL_TIM_SET_COUNTER(&htim2, 0);
	while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

uint8_t DS18B20_Start (void) {
	uint8_t Response = 0;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 0);
	delay_us(480);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 1);
	delay_us(80);
	if (!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0))) Response = 1;
	else Response = 0;
	delay_us(400);
	return Response;
}

void DS18B20_Write (uint8_t data) {
	for (int i=0; i<8; i++) {
		if ((data & (1<<i)) != 0) {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 0);
			delay_us(1);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 1);
			delay_us(60);
		} else {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 0);
			delay_us(60);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 1);
		}
	}
}

uint8_t DS18B20_Read (void) {
	uint8_t value = 0;
	for (int i=0; i<8; i++) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 0);
		delay_us(2);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 1);
		delay_us(10);
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)) value |= (1<<i);
		delay_us(50);
	}
	return value;
}

// Belirli bir ID'ye sahip sensörden veri çekme
float DS18B20_Get_Temp(uint8_t *ID) {
    uint8_t LSB, MSB;
    DS18B20_Start();
    DS18B20_Write(0x55); // Match ROM
    for (int i=0; i<8; i++) DS18B20_Write(ID[i]);
    DS18B20_Write(0xBE); // Read Scratchpad

    LSB = DS18B20_Read();
    MSB = DS18B20_Read();
    return (float)((int16_t)(MSB << 8) | LSB) / 16.0;

}
/* USER CODE END 0 */

int main(void)
{
	/* 1. Standart Donanım Başlatma */
	  HAL_Init();
	  SystemClock_Config();
	  MX_GPIO_Init();
	  MX_TIM2_Init();
	  MX_USART1_UART_Init();

	  /* 2. Kullanıcı Başlatma Alanı */
	  /* USER CODE BEGIN 2 */
	  HAL_TIM_Base_Start(&htim2); // Timer'ı başlat

	  DS18B20_Config9Bit();       // SENSÖRLERİ HIZ MODUNA AL (Kritik!)

	  HAL_UART_Transmit(&huart1, (uint8_t*)"Sistem 9-Bit Modda Basliyor...\r\n", 32, 100);

	  char msg[100];
	  /* USER CODE END 2 */

	  /* 3. Sonsuz Döngü - Maksimum Okuma Hızı */
	  while (1)
	  {
	      // A. Tüm sensörlere aynı anda "Ölçümü Başlat" emri ver
	      DS18B20_Start();
	      DS18B20_Write(0xCC); // Skip ROM
	      DS18B20_Write(0x44); // Convert T

	      // B. 9-bit modunda 100ms-150ms beklemek yeterlidir
	      HAL_Delay(100);

	      // C. Verileri ID'ler ile sırayla çek
	      T1 = DS18B20_Get_Temp(S1_ID);
	      T2 = DS18B20_Get_Temp(S2_ID);
	      T3 = DS18B20_Get_Temp(S3_ID);

	      // D. Raspberry Pi formatında paketle ve gönder
	      sprintf(msg, "S1:%.2f|S2:%.2f|S3:%.2f\n", T1, T2, T3);
	      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

	      // E. Döngü sonu bekleme (İstersen silebilirsin ama 100ms bırakmak sistemi rahatlatır)
	      HAL_Delay(100);
	  }
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration (16 MHz HSI)
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

/* Çevresel Birim Başlatma Fonksiyonları */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15; // 16MHz -> 1MHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim2);
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart1);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // Open Drain
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
// Gerekirse ek fonksiyonlar buraya
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
