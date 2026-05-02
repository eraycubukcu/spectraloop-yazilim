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
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t header;
    uint8_t slave_id;
    uint8_t data_high;
    uint8_t data_low;
    uint8_t checksum;
} SlavePacket_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SLAVE_ADDR   0x20
#define PACKET_SIZE  9
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
// slave'in göndereceği paket
uint8_t tx_buffer[PACKET_SIZE];
uint8_t S1_ID[8] = {0x28, 0x86, 0x84, 0x37, 0x00, 0x00, 0x00, 0x5C};
uint8_t S2_ID[8] = {0x28, 0xCC, 0x48, 0x38, 0x00, 0x00, 0x00, 0x7F};
uint8_t S3_ID[8] = {0x28, 0xE7, 0xB4, 0x36, 0x00, 0x00, 0x00, 0xA3};
float temps[3] = {0.0f, 0.0f, 0.0f};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
static void delay_us(uint32_t us);
static void OW_SetOutput(void);
static void OW_SetInput(void);
void        OW_Init(void);
uint8_t     OW_Reset(void);
void        OW_WriteBit(uint8_t bit);
uint8_t     OW_ReadBit(void);
void        OW_WriteByte(uint8_t byte);
uint8_t     OW_ReadByte(void);

// DS18B20 fonksiyonları
void    DS18B20_StartConversion(void);
float   DS18B20_ReadTemp(uint8_t *rom);
void    DS18B20_ReadAllTemps(void);

// Paket fonksiyonu
void PreparePacket(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// PA1 pinini open-drain output yap
static void OW_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_1;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;  // open-drain zorunlu
    GPIO_InitStruct.Pull  = GPIO_NOPULL;           // pull-up dışarıdan var (4.7kΩ)
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// PA1 pinini input yap (sensörden okuma için)
static void OW_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_1;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// 1-Wire hattını başlat
void OW_Init(void)
{
    HAL_TIM_Base_Start(&htim2);                              // TIM2 başlat
    OW_SetOutput();                                          // pin output
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     // hat HIGH (idle)
}

// Reset pulse gönder, sensörün presence pulse'ını kontrol et
// Dönüş: 1 = sensör var, 0 = sensör yok
uint8_t OW_Reset(void)
{
    uint8_t presence = 0;

    OW_SetOutput();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);   // hat LOW
    delay_us(480);                                            // 480us bekle (reset pulse)

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     // hat HIGH bırak
    OW_SetInput();
    delay_us(70);                                             // 70us sonra oku

    // Sensör varsa hattı LOW çekmiş olmalı (presence pulse)
    presence = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) ? 1 : 0;

    delay_us(410);                                            // toplam 480us tamamla
    OW_SetOutput();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     // hat HIGH bırak

    return presence;
}

// 1-Wire hattına 1 bit yaz
void OW_WriteBit(uint8_t bit)
{
    OW_SetOutput();

    if (bit)
    {
        // '1' yazmak: kısa LOW pulse
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        delay_us(6);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        delay_us(64);
    }
    else
    {
        // '0' yazmak: uzun LOW pulse
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        delay_us(60);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        delay_us(10);
    }
}

// 1-Wire hattından 1 bit oku
uint8_t OW_ReadBit(void)
{
    uint8_t bit = 0;

    OW_SetOutput();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);   // kısa LOW pulse başlat
    delay_us(6);

    OW_SetInput();                                            // hattı bırak, sensör sürsün
    delay_us(9);

    bit = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) ? 1 : 0;
    delay_us(55);                                             // slot tamamla

    return bit;
}

// 1-Wire hattına 1 byte yaz (LSB önce)
void OW_WriteByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        OW_WriteBit(byte & 0x01);   // en düşük biti yaz
        byte >>= 1;                  // sola kaydır
    }
}

// 1-Wire hattından 1 byte oku (LSB önce)
uint8_t OW_ReadByte(void)
{
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        byte |= (OW_ReadBit() << i);
    }
    return byte;
}

/* ─────────────────────────────────────────
   DS18B20 FONKSİYONLARI
   ───────────────────────────────────────── */

// Tüm sensörlerde aynı anda sıcaklık dönüşümü başlat
// Skip ROM (0xCC) → hepsine aynı anda komut gönderir
void DS18B20_StartConversion(void)
{
    OW_Reset();
    OW_WriteByte(0xCC);   // Skip ROM → tüm sensörler
    OW_WriteByte(0x44);   // Convert T → dönüşüm başlat
    // 750ms sonra okuma yapılabilir (12-bit çözünürlük)
}

// Belirli bir sensörden sıcaklık oku
// rom: 8 byte ROM ID
float DS18B20_ReadTemp(uint8_t *rom)
{
    OW_Reset();
    OW_WriteByte(0x55);              // Match ROM → belirli sensörü seç
    for (uint8_t i = 0; i < 8; i++)
        OW_WriteByte(rom[i]);        // ROM ID gönder

    OW_WriteByte(0xBE);              // Read Scratchpad → veriyi oku

    uint8_t lsb = OW_ReadByte();    // düşük byte
    uint8_t msb = OW_ReadByte();    // yüksek byte

    int16_t raw = (msb << 8) | lsb;
    return raw / 16.0f;              // 12-bit → °C'ye çevir
}

// 3 sensörden sıcaklık oku, temps[] dizisine yaz
void DS18B20_ReadAllTemps(void)
{
    temps[0] = DS18B20_ReadTemp(S1_ID);
    temps[1] = DS18B20_ReadTemp(S2_ID);
    temps[2] = DS18B20_ReadTemp(S3_ID);
}

/* ─────────────────────────────────────────
   I2C PAKET HAZIRLAMA
   ───────────────────────────────────────── */

// Paket yapısı (9 byte):
// [0]    Header    = 0xAA
// [1]    Slave ID  = 0x20
// [2-3]  Temp1     = int16 (°C × 100)
// [4-5]  Temp2     = int16 (°C × 100)
// [6-7]  Temp3     = int16 (°C × 100)
// [8]    Checksum  = XOR([0]...[7])
void PreparePacket(void)
{
    // Sıcaklıkları int16'ya çevir (örn: 23.45°C → 2345)
    int16_t t1 = (int16_t)(temps[0] * 10);
    int16_t t2 = (int16_t)(temps[1] * 10);
    int16_t t3 = (int16_t)(temps[2] * 10);

    tx_buffer[0] = 0xAA;                    // header
    tx_buffer[1] = SLAVE_ADDR;              // slave id

    tx_buffer[2] = (t1 >> 8) & 0xFF;       // Temp1 high byte
    tx_buffer[3] =  t1 & 0xFF;             // Temp1 low byte

    tx_buffer[4] = (t2 >> 8) & 0xFF;       // Temp2 high byte
    tx_buffer[5] =  t2 & 0xFF;             // Temp2 low byte

    tx_buffer[6] = (t3 >> 8) & 0xFF;       // Temp3 high byte
    tx_buffer[7] =  t3 & 0xFF;             // Temp3 low byte

    // Checksum: tüm byte'ların XOR'u
    tx_buffer[8] = tx_buffer[0]
                 ^ tx_buffer[1]
                 ^ tx_buffer[2]
                 ^ tx_buffer[3]
                 ^ tx_buffer[4]
                 ^ tx_buffer[5]
                 ^ tx_buffer[6]
                 ^ tx_buffer[7];
}
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
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	  // İlk paketi hazırla, master istek yapana kadar bekle
  // 1-Wire hattını başlat
   OW_Init();

   // İlk dönüşümü başlat ve 750ms bekle
   DS18B20_StartConversion();
   HAL_Delay(750);

   // 3 sensörden ilk sıcaklık okumasını yap
   DS18B20_ReadAllTemps();

   // İlk paketi hazırla ve master istek yapana kadar bekle
   PreparePacket();
   HAL_I2C_Slave_Transmit_IT(&hi2c1, tx_buffer, PACKET_SIZE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	  while (1)
	  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	    // Her 500ms'de bir yeni paket hazırla
		  DS18B20_StartConversion();
		      HAL_Delay(750);              // 750ms dönüşüm süresi bekle

		      // 3 sensörden sıcaklık oku
		      DS18B20_ReadAllTemps();

		      // Paketi güncelle
		      PreparePacket();
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 32;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xFFFFFFFF;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Gönderim tamamlanınca tekrar dinlemeye geç
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        HAL_I2C_Slave_Transmit_IT(&hi2c1, tx_buffer, PACKET_SIZE);
    }
}

// Hata olursa tekrar dinlemeye geç
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        HAL_I2C_Slave_Transmit_IT(&hi2c1, tx_buffer, PACKET_SIZE);
    }
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
