/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 ** This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * COPYRIGHT(c) 2018 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Includes */
#include "stm32f1xx_it.h"
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "matrix.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
/* TODO */
#define DEBUG					1
#if DEBUG
#define RGB_PANEL_DEBUG			0
#endif	//if DEBUG

#define HW_VERSION				"v1.0.0"
#define SW_VERSION				"v1.3.0"
#define RUNNING_SPEED			25

#define OE_MIN					((OE_MAX_DUTY * 5) / 1000)
#define UART_BUFSIZE			1024

#define CMD_CHAR_HEADER			'$'
#define CMD_CHAR_ENDLINE		'\n'
#define CMD_CHAR_SEPARATOR		','
#define CMD_CHAR_TERMINATOR		'*'

const char bangladesh_eng[16] = "Bangladesh";
const char railways_eng[16] = "Railways";
const char bangladesh_bangla[16] = { 34, 50, 55, 38, 50, 59, 29, 40 };
const char railways_bangla[16] = { 59, 35, 38, 10, 59, 63 };

char uartBuffer[UART_BUFSIZE];
volatile ITStatus uart1RxReady = RESET;
volatile uint8_t uart1RxPointer = 0;
volatile uint8_t swapBufferStart = 0;
volatile uint32_t activeFrameRowAdress = 0;
volatile uint32_t matrix_row = 0;
volatile uint32_t matrix_color = 0;
volatile FlagStatus buttonPress = RESET;
#if DEBUG
volatile uint32_t fps = 0;
#endif	//if DEBUG

uint32_t millis = 0;
const uint32_t DISCONNECTED_TIMEOUT = 10000;
uint32_t disconnectTimer = 0;
const uint32_t DISCONNECTED_REFRESH_TIMEOUT = 5000;
uint32_t disconnectRefreshTimer = 0;
const uint32_t DISPLAY_REFRESH_TIMEOUT = 5000;
uint32_t displayRefreshTimer = 0;

const uint32_t OE_MAX_VAL = OE_MAX_DUTY - OE_MIN;
const uint32_t OE_MIN_VAL = (OE_MAX_DUTY * 85) / 100;
uint32_t oeCcrVal = OE_MAX_DUTY;

#define COMMAND_SHORT_BUFSIZE			16
#define COMMAND_LONG_BUFSIZE			128

typedef enum
{
	STATION_ARRIVED = 0,
	STATION_TOWARD = 1,
	STATION_NEXT = 0,
	STATION_PREV = 1
} Station_State_e;

typedef struct
{
	char id[COMMAND_SHORT_BUFSIZE];
	char name[COMMAND_LONG_BUFSIZE];
} Train_Info_t;
typedef struct
{
	char name[COMMAND_LONG_BUFSIZE];
	Station_State_e state;
	char first[COMMAND_LONG_BUFSIZE];
	char end[COMMAND_LONG_BUFSIZE];
} Station_Info_t;

char textRecv[COMMAND_SHORT_BUFSIZE];

/**
 * @brief  Display Command Handle Structure definition
 */
typedef struct
{
	Train_Info_t trainInfo;
	Station_Info_t stationInfo;
	char coachName[COMMAND_SHORT_BUFSIZE];
	FlagStatus updateAvailable;
} CMD_HandleTypeDef;

CMD_HandleTypeDef infoEng, infoBangla, infoDisplay;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
/* TODO */
static void layoutConnected(uint8_t *lang);
static void layoutDisconnected(uint8_t * lang);
static void layoutCoachName(uint8_t len);

static uint8_t char_to_byte(char c);
static void parseCommand();

void startDmaTransfering();
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void dmaTransferCompleted(DMA_HandleTypeDef *hdma);
void usart1_callback_IT();
static void delayIwdg(uint32_t _delayTime);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 *
 * @retval None
 */
int main(void)
{
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

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
	MX_TIM1_Init();
	MX_TIM2_Init();
	MX_USART1_UART_Init();
	MX_IWDG_Init();
	/* USER CODE BEGIN 2 */
	/* TODO */
	HAL_IWDG_Refresh(&hiwdg);
	/* init data */
	rgb_init();

	activeFrameRowAdress = rgb_get_buffer(0);

	/* enable tim1 dma */
	__HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_CC1);
	hdma_tim1_ch1.XferCpltCallback = dmaTransferCompleted;

	/* enable tim3 */
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	htim2.Instance->CCR1 = OE_MAX_VAL;

	__HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

	HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_RESET);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	uint8_t clearScreenForUpload = 0;
	uint8_t displayLang = 0;

	while (1)
	{

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		/* TODO */

		millis = HAL_GetTick();
		HAL_IWDG_Refresh(&hiwdg);

		parseCommand();

		if (!clearScreenForUpload)
		{
			if (millis >= disconnectTimer)
			{
				if (millis >= disconnectRefreshTimer)
				{
					disconnectRefreshTimer = millis + DISCONNECTED_REFRESH_TIMEOUT;
					memset(infoEng.coachName, 0, COMMAND_SHORT_BUFSIZE);
					memset(infoBangla.coachName, 0, COMMAND_SHORT_BUFSIZE);
					displayRefreshTimer = 0;

					layoutDisconnected(&displayLang);
				}
			}
			else
			{
				if (millis >= displayRefreshTimer)
				{
					displayRefreshTimer = millis + DISPLAY_REFRESH_TIMEOUT;

					layoutConnected(&displayLang);
				}

			}
		}

		if (buttonPress == SET)
		{
			rgb_frame_clear();
			swapBufferStart = 1;
			clearScreenForUpload = 1;
			HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_SET);
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

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1
			| RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure the Systick interrupt time
	 */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

	/**Configure the Systick
	 */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* IWDG init function */
static void MX_IWDG_Init(void)
{

	hiwdg.Instance = IWDG;
	hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
	hiwdg.Init.Reload = IWDG_1S;
	if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;
	TIM_OC_InitTypeDef sConfigOC;
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

	htim1.Instance = TIM1;
	htim1.Init.Prescaler = CLK_PSC;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = CLK_ARR;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = CLK_MAX_DUTY / 2;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	HAL_TIM_MspPostInit(&htim1);

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;
	TIM_OC_InitTypeDef sConfigOC;

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = OE_PSC;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = OE_ARR;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = OE_MAX_DUTY;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	HAL_TIM_MspPostInit(&htim2);

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

	huart1.Instance = USART1;
	huart1.Init.BaudRate = 38400;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/** 
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE()
	;

	/* DMA interrupt init */
	/* DMA1_Channel2_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/** Configure pins as 
 * Analog
 * Input
 * Output
 * EVENT_OUT
 * EXTI
 * Free pins are configured automatically as Analog (this feature is enabled through
 * the Code Generation settings)
 */
static void MX_GPIO_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStruct;

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE()
	;
	__HAL_RCC_GPIOD_CLK_ENABLE()
	;
	__HAL_RCC_GPIOA_CLK_ENABLE()
	;
	__HAL_RCC_GPIOB_CLK_ENABLE()
	;

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin, GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, r1_Pin | g1_Pin | b1_Pin | r2_Pin | g2_Pin | b2_Pin | clk_en_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, da_Pin | db_Pin | dc_Pin | dd_Pin | stb_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : LED_BUILTIN_Pin */
	GPIO_InitStruct.Pin = LED_BUILTIN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(LED_BUILTIN_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : btn_Pin */
	GPIO_InitStruct.Pin = btn_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(btn_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PC15 */
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : r1_Pin g1_Pin b1_Pin r2_Pin
	 g2_Pin b2_Pin */
	GPIO_InitStruct.Pin = r1_Pin | g1_Pin | b1_Pin | r2_Pin | g2_Pin | b2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PA6 PA11 PA12 */
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : clk_en_Pin */
	GPIO_InitStruct.Pin = clk_en_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(clk_en_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : da_Pin db_Pin dc_Pin dd_Pin
	 stb_Pin */
	GPIO_InitStruct.Pin = da_Pin | db_Pin | dc_Pin | dd_Pin | stb_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : PB10 PB11 PB12 PB13
	 PB14 PB15 PB5 PB6
	 PB7 PB8 PB9 */
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14
			| GPIO_PIN_15 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
/* TODO Non-hardware functions*/

static void layoutConnected(uint8_t *lang)
{
	uint8_t u8a, u8b;
	uint8_t displayLang = *lang;

	if (displayLang)
	{
		/* english */
		infoDisplay = infoEng;

		u8a = strlen(infoDisplay.coachName);
		/* no data at english info */
		if (!u8a)
		{
			u8b = strlen(infoBangla.coachName);
			/* no data at bangla info either */
			if (!u8b)
			{
				//change to disconnect display
				disconnectTimer = 0;
			}
			else
			{
				displayRefreshTimer = 0;
			}
		}
		else
		{
			rgb_frame_clear();
			layoutCoachName(u8a);

			swapBufferStart = 1;
		}

		*lang = 0;
	}
	else
	{
		/* bangla */
		infoDisplay = infoBangla;

		u8a = strlen(infoDisplay.coachName);
		/* no data at bangla info */
		if (!u8a)
		{
			u8b = strlen(infoEng.coachName);
			/* no data at bangla info either */
			if (!u8b)
			{
				//change to disconnect display
				disconnectTimer = 0;
			}
			else
			{
				displayRefreshTimer = 0;
			}
		}
		else
		{
			rgb_frame_clear();
			layoutCoachName(u8a);

			swapBufferStart = 1;
		}

		*lang = 1;
	}

}

static void layoutDisconnected(uint8_t * lang)
{
	uint8_t displayLang = *lang;
	int16_t xPos = 0;

	rgb_frame_clear();
	if (displayLang)
	{
		/* english */
		xPos = 36;
		rgb_print(xPos, 0, (char *) bangladesh_eng, strlen(bangladesh_eng), 0b1, 2);
		rgb_print(xPos + 12, 16, (char *) railways_eng, strlen(railways_eng), 0b111, 2);

		*lang = 0;
	}
	else
	{
		/* bangla */
		xPos = 56;
		rgb_bangla_print(xPos, 0, (char *) bangladesh_bangla, strlen(bangladesh_bangla), 0b1, 1);
		rgb_bangla_print(xPos + 10, 16, (char *) railways_bangla, strlen(railways_bangla), 0b111,
				1);

		*lang = 1;
	}
	swapBufferStart = 1;
}

static void layoutCoachName(uint8_t len)
{

	if (len == 1)
		rgb_write(0, 2, infoDisplay.coachName[0], 0b1, 4);
	else if (len == 2)
		rgb_print(0, 4, infoDisplay.coachName, 2, 0b1, 3);
	else if (len == 3)
	{
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(6, 16, infoDisplay.coachName[2], 0b1, 2);
	}
	else if (len == 4)
	{
		//xtr1
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(24, 0, infoDisplay.coachName[2], 0b1, 2);
		rgb_write(12, 16, infoDisplay.coachName[3], 0b1, 2);
	}
	else if (len == 5)
	{
		//xtr01
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(24, 0, infoDisplay.coachName[2], 0b1, 2);
		rgb_write(6, 16, infoDisplay.coachName[3], 0b1, 2);
		rgb_write(18, 16, infoDisplay.coachName[4], 0b1, 2);
	}
	else if (len == 6)
	{
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(24, 0, infoDisplay.coachName[2], 0b1, 2);
		rgb_write(0, 16, infoDisplay.coachName[3], 0b1, 2);
		rgb_write(12, 16, infoDisplay.coachName[4], 0b1, 2);
		rgb_write(24, 16, infoDisplay.coachName[5], 0b1, 2);
	}
}

static uint8_t char_to_byte(char c)
{
	uint8_t ret = 0;

	if (c >= '0' && c <= '9')
		ret = c - '0';
	else if (c >= 'a' && c <= 'f')
		ret = c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		ret = c - 'A' + 10;

	return ret;
}

static void parseCommand()
{
	char cmdIn[UART_BUFSIZE];
	char buf[COMMAND_SHORT_BUFSIZE];
	uint8_t crcVal = 0, crcCalc = 0;
	uint16_t cmdIn_length = 0;
	uint16_t cmdCounter = 0;
	char * s;
	uint16_t awal = 0;
	CMD_HandleTypeDef cmdTem;
	int i;

	if (uart1RxReady == SET)
	{
		memcpy(cmdIn, uartBuffer, UART_BUFSIZE);
		uart1RxReady = RESET;

		/*
		 * start parsing data
		 *
		 * $[counter],[trainID],[trainName],[stationName],[stationState],
		 * [station first],[station end],[coachName]*[CRC]\n
		 *
		 */
		cmdIn_length = strlen(cmdIn);
		for ( i = 1; i < cmdIn_length; i++ )
		{
			if (cmdIn[i] == CMD_CHAR_TERMINATOR)
			{
				awal = i;
				break;
			}
			else
				crcCalc ^= (uint8_t) cmdIn[i];
		}
		crcVal = char_to_byte(cmdIn[awal + 1]) << 4;
		crcVal |= char_to_byte(cmdIn[awal + 2]);

		/* valid CRC */
		if (crcCalc == crcVal)
		{
			disconnectTimer = millis + DISCONNECTED_TIMEOUT;
			disconnectRefreshTimer = 0;

			i = 0;
			/* counter */
			awal = i + 1;
			memset(buf, 0, COMMAND_SHORT_BUFSIZE);
			s = buf;
			for ( i = awal; i < cmdIn_length; i++ )
			{
				if (cmdIn[i] == CMD_CHAR_SEPARATOR)
					break;
				else
					*s++ = cmdIn[i];
			}
			cmdCounter = atoi(buf);

			/* trainID */
			awal = i + 1;
			memset(cmdTem.trainInfo.id, 0, COMMAND_SHORT_BUFSIZE);
			s = cmdTem.trainInfo.id;
			for ( i = awal; i < cmdIn_length; i++ )
			{
				if (cmdIn[i] == CMD_CHAR_SEPARATOR)
					break;
				else
					*s++ = cmdIn[i];
			}

			/* trainName */
			awal = i + 1;
			memset(cmdTem.trainInfo.name, 0, COMMAND_LONG_BUFSIZE);
			s = cmdTem.trainInfo.name;
			for ( i = awal; i < cmdIn_length; i++ )
			{
				if (cmdIn[i] == CMD_CHAR_SEPARATOR)
					break;
				else
					*s++ = cmdIn[i];
			}

			/* stationName */
			awal = i + 1;
			memset(cmdTem.stationInfo.name, 0, COMMAND_LONG_BUFSIZE);
			s = cmdTem.stationInfo.name;
			for ( i = awal; i < cmdIn_length; i++ )
			{
				if (cmdIn[i] == CMD_CHAR_SEPARATOR)
					break;
				else
					*s++ = cmdIn[i];
			}
			/* station state */
			awal = i + 1;

			if (cmdIn[awal] == '0')
				cmdTem.stationInfo.state = STATION_ARRIVED;
			else if (cmdIn[awal] == '1')
				cmdTem.stationInfo.state = STATION_TOWARD;

			i = awal + 1;

			/* station first */
			awal = i + 1;
			memset(cmdTem.stationInfo.first, 0, COMMAND_LONG_BUFSIZE);
			s = cmdTem.stationInfo.first;
			for ( i = awal; i < cmdIn_length; i++ )
			{
				if (cmdIn[i] == CMD_CHAR_SEPARATOR)
					break;
				else
					*s++ = cmdIn[i];
			}
			/* station end */
			awal = i + 1;
			memset(cmdTem.stationInfo.end, 0, COMMAND_LONG_BUFSIZE);
			s = cmdTem.stationInfo.end;
			for ( i = awal; i < cmdIn_length; i++ )
			{
				if (cmdIn[i] == CMD_CHAR_SEPARATOR)
					break;
				else
					*s++ = cmdIn[i];
			}
			/* coachName */
			awal = i + 1;
			memset(cmdTem.coachName, 0, COMMAND_SHORT_BUFSIZE);
			s = cmdTem.coachName;
			for ( i = awal; i < cmdIn_length; i++ )
			{
				if (cmdIn[i] == CMD_CHAR_TERMINATOR)
					break;
				else
					*s++ = cmdIn[i];
			}

			cmdTem.updateAvailable = SET;
			if (cmdCounter % 2 == 1)
				infoBangla = cmdTem;
			else
				infoEng = cmdTem;

			HAL_GPIO_TogglePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin);
		}
	}

}

/* TODO */

void startDmaTransfering()
{
	HAL_DMA_Start_IT(&hdma_tim1_ch1, activeFrameRowAdress, (uint32_t) &DATA_GPIO->ODR, FRAME_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_tim1_ch1, DMA_IT_HT);

	/* Start TIM PWM channel 1 */
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void dmaTransferCompleted(DMA_HandleTypeDef *hdma)
{
	if (hdma->Instance == DMA1_Channel2)
	{
		DATA_GPIO->BSRR = DATA_BSSR_CLEAR;
		HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

		/* update matrix row */
		if (++matrix_row >= MATRIX_SCANROW)
		{
			matrix_row = 0;
			if (swapBufferStart)
			{
				rgb_swap_buffer();
				swapBufferStart = 0;
			}

#if DEBUG
			fps++;
#endif	//if DEBUG

		}
		activeFrameRowAdress = rgb_get_buffer(matrix_row);

		/* set stb low */
		CONTROL_GPIO->BSRR = CONTROL_BSSR_STB_CLEAR;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* set stb high */
	/* set matrix row */
	CONTROL_GPIO->ODR = (matrix_row << CONTROL_SHIFTER) | stb_Pin;
	startDmaTransfering();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_14)
	{
		buttonPress = SET;
		HAL_GPIO_TogglePin(LED_BUILTIN_GPIO_Port, LED_BUILTIN_Pin);
	}
}

void usart1_callback_IT()
{
	uint32_t isrflags = READ_REG(huart1.Instance->SR);
	char c;

	/* RXNE handler */
	if ((isrflags & USART_SR_RXNE) != RESET)
	{
		c = (char) (huart1.Instance->DR & (uint8_t) 0x00FF);
		if (c == CMD_CHAR_HEADER)
		{
			memset(uartBuffer, 0, UART_BUFSIZE);
			uart1RxPointer = 0;
		}
		else if (c == CMD_CHAR_ENDLINE)
			uart1RxReady = SET;

		uartBuffer[uart1RxPointer++] = c;
	}

	/* ------------------------------------------------------------ */
	/* Other USART1 interrupts handler can go here ...             */
}

static void delayIwdg(uint32_t _delayTime)
{
	const uint32_t iwdg_reset_time = 100;

	HAL_IWDG_Refresh(&hiwdg);
	if (_delayTime <= iwdg_reset_time)
		HAL_Delay(_delayTime);
	else
	{
		while (_delayTime > iwdg_reset_time)
		{
			HAL_Delay(iwdg_reset_time);
			HAL_IWDG_Refresh(&hiwdg);
			_delayTime -= iwdg_reset_time;
		}
		HAL_Delay(_delayTime);
	}
	HAL_IWDG_Refresh(&hiwdg);
}

/* TODO */
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  file: The file name as string.
 * @param  line: The line in file as a number.
 * @retval None
 */
void _Error_Handler(char *file, int line)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
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
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
