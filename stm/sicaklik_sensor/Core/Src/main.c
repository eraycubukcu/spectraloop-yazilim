/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DS18B20 Telemetry System - 3 Sensors + Loop Hz + Uptime
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"

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
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// Sensör Kimlikleri (Sabit)
uint8_t S1_ID[8] = {0x28, 0x86, 0x84, 0x37, 0x00, 0x00, 0x00, 0x5C};
uint8_t S2_ID[8] = {0x28, 0xCC, 0x48, 0x38, 0x00, 0x00, 0x00, 0x7F};
uint8_t S3_ID[8] = {0x28, 0xE7, 0xB4, 0x36, 0x00, 0x00, 0x00, 0xA3};

// Sıcaklık ve Telemetri Değişkenleri
float T1 = 0, T2 = 0, T3 = 0;
float Loop_Hz = 0;
uint32_t last_tick = 0;
uint32_t current_uptime = 0;

char msg_buffer[150];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void delay_us (uint16_t us);
uint8_t DS18B20_Start (void);
void DS18B20_Write (uint8_t data);
uint8_t DS18B20_Read (void);
void DS18B20_Config9Bit(void);
float DS18B20_Read_By_ID(uint8_t *ID);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay_us (uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void DS18B20_Config9Bit(void) {
    if (DS18B20_Start()) {
        DS18B20_Write(0xCC); // Skip ROM
        DS18B20_Write(0x4E); // Write Scratchpad
        DS18B20_Write(0x00); // TH
        DS18B20_Write(0x00); // TL
        DS18B20_Write(0x1F); // 9-bit resolution
    }
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

float DS18B20_Read_By_ID(uint8_t *ID) {
    uint8_t b1, b2;
    if (DS18B20_Start()) {
        DS18B20_Write(0x55); // Match ROM
        for (int i=0; i<8; i++) DS18B20_Write(ID[i]);
        DS18B20_Write(0xBE); // Read Scratchpad
        b1 = DS18B20_Read();
        b2 = DS18B20_Read();
        return (float)((int16_t)(b2 << 8) | b1) / 16.0;
    }
    return -99.0;
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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  DS18B20_Config9Bit(); // Sensörleri hızlandır

  HAL_UART_Transmit(&huart1, (uint8_t*)"Telemetri Basliyor...\r\n", 23, 100);
  last_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  // --- 1. FREKANS (Hz) VE UPTIME HESAPLAMA ---
	        uint32_t current_time = HAL_GetTick();
	        uint32_t delta = current_time - last_tick;

	        if(delta > 0) {
	            Loop_Hz = 1000.0f / delta; // Döngü kaç Hz ile dönüyor?
	        }
	        last_tick = current_time;
	        current_uptime = current_time / 1000; // Saniye cinsinden çalışma süresi

	        // --- 2. SENSÖRLERE ÖLÇÜM EMRİ VER (SKIP ROM) ---
	        DS18B20_Start();
	        DS18B20_Write(0xCC); // Tüm sensörlere seslen
	        DS18B20_Write(0x44); // Sıcaklığı ölçmeye başlayın (Convert T)

	        // 9-bit modunda 100ms beklemek yeterlidir (Hızlı okuma için)
	        HAL_Delay(100);

	        // --- 3. VERİLERİ KİMLİKLERİNE (ID) GÖRE TEK TEK ÇEK ---
	        T1 = DS18B20_Read_By_ID(S1_ID);
	        T2 = DS18B20_Read_By_ID(S2_ID);
	        T3 = DS18B20_Read_By_ID(S3_ID);

	        // --- 4. RASPBERRY PI İÇİN PAKETLE VE GÖNDER (USART2!) ---
	        char msg[128];
	        // Format: S1:24.50|S2:25.10|S3:23.85|HZ:9.50|UP:123
	        int len = sprintf(msg, "S1:%.2f|S2:%.2f|S3:%.2f|HZ:%.2f|UP:%lu\n",
	                          T1, T2, T3, Loop_Hz, (unsigned long)current_uptime);

	        // USB üzerinden göndermek için huart2 kullanıyoruz
	        HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 100);

	        // --- 5. SİSTEMİ BİRAZ RAHATLAT (OPSİYONEL) ---
	        // Arayüzün saniyede 10 paket (10Hz) alması yeterlidir.
	        HAL_Delay(10);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
