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
DMA_HandleTypeDef hdma_tim1_up;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
/* TODO */
#define DEBUG					0
#if DEBUG
#define RGB_PANEL_DEBUG			0
#endif	//if DEBUG

#define HW_VERSION				"v1.2.3"
#define SW_VERSION				"v1.3.7a"
#define RUNNING_SPEED			25

#if DISPLAY_OUTDOOR
#define OE_MIN					((OE_MAX_DUTY * 50) / 1000)
#else
#define OE_MIN					((OE_MAX_DUTY * 500) / 1000)
#endif	//if DISPLAY_OUTDOOR

#define UART_BUFSIZE			1024

#define CMD_CHAR_HEADER			'$'
#define CMD_CHAR_ENDLINE		'\n'
#define CMD_CHAR_SEPARATOR		','
#define CMD_CHAR_TERMINATOR		'*'

#define COMMAND_SHORT_BUFSIZE			16
#define COMMAND_LONG_BUFSIZE			128

typedef struct
{
	char buffer[UART_BUFSIZE];
	volatile uint16_t head;
	volatile uint16_t tail;
	uint16_t size;
} Ring_Buffer_t;

const char bangladesh_eng[COMMAND_SHORT_BUFSIZE] = "Bangladesh";
const char railways_eng[COMMAND_SHORT_BUFSIZE] = "Railways";
const char bangladesh_bangla[COMMAND_SHORT_BUFSIZE] = { 34, 50, 55, 38, 50, 59, 29, 40 };
const char railways_bangla[COMMAND_SHORT_BUFSIZE] = { 59, 35, 38, 10, 59, 63 };
#if DISPLAY_OUTDOOR
const char to_in_bangla[COMMAND_SHORT_BUFSIZE] = { 6, 59, 28, 59, 56, 6 };
const char to_in_eng[COMMAND_SHORT_BUFSIZE] = { ' ', 't', 'o', ' ' };
#endif	//if DISPLAY_OUTDOOR

#if DISPLAY_INDOOR
const char at_in_bangla[COMMAND_SHORT_BUFSIZE] =
{	73, 50, 14, 27, 6};
const char at_in_eng[COMMAND_SHORT_BUFSIZE] =
{	'A', 't', ' '};
const char to_in_bangla[COMMAND_SHORT_BUFSIZE] =
{	64, 50, 51, 17, 18, 6};
const char to_in_eng[COMMAND_SHORT_BUFSIZE] =
{	'T', 'o', ' '};
#endif	//if DISPLAY_INDOOR

//char uartBuffer[UART_BUFSIZE];
Ring_Buffer_t uartBuffer = { { 0 }, 0, 0, UART_BUFSIZE };
volatile ITStatus uart1RxReady = RESET;
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
const uint32_t DISCONNECTED_REFRESH_TIMEOUT = 5000;
const uint32_t DISPLAY_REFRESH_TIMEOUT = 30000;
int16_t xCoachLineEnd = 0;
int16_t xTrainIdLineEnd = 0;
const uint32_t ANIMATION_START_TIMEOUT = 1000;
const uint32_t ANIMATION_RUNNING_TIMEOUT = 35;

const uint32_t OE_MAX_VAL = OE_MAX_DUTY - OE_MIN;
const uint32_t OE_MIN_VAL = (OE_MAX_DUTY * 85) / 100;
uint32_t oeCcrVal = OE_MAX_DUTY;

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
	uint8_t language;
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
static uint8_t layoutConnected(uint8_t *lang);
static void layoutConnectedValid(uint8_t coach_len);
static void layoutDisconnected(uint8_t * lang);
static void layoutCoachName(uint8_t len);
static void layoutTrainID(int16_t xOffset);
static int16_t layoutTrainName(int16_t xOffset);
static int16_t layoutTrainRoute(int16_t xOffset);

static uint8_t char_to_byte(char c);
static uint8_t parseCommand();
static void parseBangla(CMD_HandleTypeDef *cmd);

static void startDmaTransfering();
static void dmaTransferCompleted(DMA_HandleTypeDef *hdma);
static void ring_buffer_write(Ring_Buffer_t *buffer, char c);
static HAL_StatusTypeDef ring_buffer_read(Ring_Buffer_t *buffer, char *c);
static HAL_StatusTypeDef ring_buffer_readString(Ring_Buffer_t *buffer, char *to, uint16_t size,
		char start, char end);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

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
	char initDisplay[COMMAND_SHORT_BUFSIZE];
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
	__HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_UPDATE);
	hdma_tim1_up.XferCpltCallback = dmaTransferCompleted;
	/* start timer 1 */
	HAL_TIM_Base_Start(&htim1);

	/* enable tim3 */
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	htim2.Instance->CCR1 = OE_MAX_VAL;

	__HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

	HAL_IWDG_Refresh(&hiwdg);

	while (1)
	{
		HAL_IWDG_Refresh(&hiwdg);
		rgb_frame_clear();
		rgb_draw_pixel(0, 0, 0b100);
		rgb_draw_pixel(63, 31, 0b010);
		swapBufferStart = 1;

		delayIwdg(1000);

		for ( int yPos = 0; yPos < MATRIX_MAX_HEIGHT; yPos++ )
		{
			for ( int xPos = 0; xPos < MATRIX_MAX_WIDTH; xPos++ )
			{
				rgb_frame_clear();
				rgb_draw_pixel(xPos, yPos, 0b1);
				swapBufferStart = 1;

				delayIwdg(10);
			}
		}

	}

	sprintf(initDisplay, "Respati");
	rgb_print(0, 0, initDisplay, strlen(initDisplay), 0b1, 1);
	sprintf(initDisplay, "h:%s", HW_VERSION);
	rgb_print(0, 8, initDisplay, strlen(initDisplay), 0b1, 1);
	sprintf(initDisplay, "s:%s", SW_VERSION);
	rgb_print(0, 16, initDisplay, strlen(initDisplay), 0b1, 1);
	swapBufferStart = 1;
	delayIwdg(2000);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	uint8_t displayLang = 0;
	uint32_t disconnectTimer = 0;
	uint32_t disconnectRefreshTimer = 0;
	uint32_t displayRefreshTimer = 0;
	uint8_t layoutState = 0;
	uint8_t animationState = 0;
	uint32_t animationTimer = 0;
	int16_t xTrainNameOffset = 0;
	int16_t xTrainRouteOffset = 0;
	int16_t xTrainNameEnd = 0;
	int16_t xTrainRouteEnd = 0;
	uint8_t u8a;

	while (1)
	{

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		/* TODO */

		millis = HAL_GetTick();
		HAL_IWDG_Refresh(&hiwdg);

		if (parseCommand())
		{
			disconnectTimer = millis + DISCONNECTED_TIMEOUT;
			disconnectRefreshTimer = 0;
		}

		if (millis >= disconnectTimer)
		{
			if (disconnectRefreshTimer == 0)
			{
				rgb_frame_clear();
				swapBufferStart = 1;
				delayIwdg(500);
			}
			if (millis >= disconnectRefreshTimer)
			{
				disconnectRefreshTimer = millis + DISCONNECTED_REFRESH_TIMEOUT;
				memset(infoEng.coachName, 0, COMMAND_SHORT_BUFSIZE);
				memset(infoBangla.coachName, 0, COMMAND_SHORT_BUFSIZE);
				displayRefreshTimer = 0;
				animationState = 0;

				layoutDisconnected(&displayLang);
			}
		}
		else
		{
			if (millis >= displayRefreshTimer)
			{
				displayRefreshTimer = millis + DISPLAY_REFRESH_TIMEOUT;

				layoutState = layoutConnected(&displayLang);
				if (layoutState == 1)
					displayRefreshTimer = 0;
				else if (layoutState == 0)
					//change to disconnect display
					disconnectTimer = 0;
				else if (layoutState == 2)
				{
					animationState = 1;
					animationTimer = millis + ANIMATION_START_TIMEOUT;
					xTrainNameOffset = 0;
					xTrainRouteOffset = 0;
				}
			}

			if (animationState)
			{
				if (millis >= animationTimer && !swapBufferStart)
				{
					animationTimer = millis + ANIMATION_RUNNING_TIMEOUT;

					u8a = strlen(infoDisplay.coachName);
					rgb_frame_clear();

					layoutCoachName(u8a);
#if DISPLAY_OUTDOOR
					layoutTrainID(xTrainNameOffset);
					xTrainNameEnd = layoutTrainName(xTrainNameOffset);
					if (xTrainNameEnd >= MATRIX_MAX_WIDTH)
						xTrainNameOffset++;
					xTrainRouteEnd = layoutTrainRoute(xTrainRouteOffset);
					if (xTrainRouteEnd >= MATRIX_MAX_WIDTH)
						xTrainRouteOffset++;
#elif DISPLAY_INDOOR
					layoutTrainID(xTrainNameOffset);
					xTrainNameEnd = layoutTrainName(xTrainNameOffset);
					if (xTrainNameEnd >= MATRIX_MAX_WIDTH)
					xTrainNameOffset++;
					xTrainRouteEnd = layoutTrainRoute(xTrainRouteOffset);
					if (xTrainRouteEnd >= MATRIX_MAX_WIDTH)
					xTrainRouteOffset++;
#endif	//if DISPLAY_OUTDOOR

					if (xTrainNameEnd <= MATRIX_MAX_WIDTH && xTrainRouteEnd <= MATRIX_MAX_WIDTH)
					{
						animationState = 0;
						displayRefreshTimer = millis + 1000;
					}

					swapBufferStart = 1;
				}
			}
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

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

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
	huart1.Init.BaudRate = CMD_BAUD;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_RX;
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
	/* DMA1_Channel5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 1, 1);
	HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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
	HAL_GPIO_WritePin(GPIOA, r1_Pin | g1_Pin | b1_Pin | r2_Pin | g2_Pin | b2_Pin | clk_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, da_Pin | db_Pin | dc_Pin | dd_Pin | stb_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : LED_BUILTIN_Pin */
	GPIO_InitStruct.Pin = LED_BUILTIN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(LED_BUILTIN_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : PC14 PC15 */
	GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : r1_Pin g1_Pin b1_Pin r2_Pin
	 g2_Pin b2_Pin clk_Pin */
	GPIO_InitStruct.Pin = r1_Pin | g1_Pin | b1_Pin | r2_Pin | g2_Pin | b2_Pin | clk_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PA6 PA8 PA11 PA12 */
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

}

/* USER CODE BEGIN 4 */
/* TODO Non-hardware functions*/

static void layoutCoachName(uint8_t len)
{

	if (len == 1)
	{
		rgb_write(0, 2, infoDisplay.coachName[0], 0b1, 4);
		xCoachLineEnd = 22;
	}
	else if (len == 2)
	{
		rgb_print(0, 4, infoDisplay.coachName, 2, 0b1, 3);
		xCoachLineEnd = 36;
	}
	else if (len == 3)
	{
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(6, 16, infoDisplay.coachName[2], 0b1, 2);
		xCoachLineEnd = 26;
	}
	else if (len == 4)
	{
		//xtr1
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(24, 0, infoDisplay.coachName[2], 0b1, 2);
		rgb_write(12, 16, infoDisplay.coachName[3], 0b1, 2);
		xCoachLineEnd = 38;
	}
	else if (len == 5)
	{
		//xtr01
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(24, 0, infoDisplay.coachName[2], 0b1, 2);
		rgb_write(6, 16, infoDisplay.coachName[3], 0b1, 2);
		rgb_write(18, 16, infoDisplay.coachName[4], 0b1, 2);
		xCoachLineEnd = 38;
	}
	else if (len == 6)
	{
		rgb_write(0, 0, infoDisplay.coachName[0], 0b1, 2);
		rgb_write(12, 0, infoDisplay.coachName[1], 0b1, 2);
		rgb_write(24, 0, infoDisplay.coachName[2], 0b1, 2);
		rgb_write(0, 16, infoDisplay.coachName[3], 0b1, 2);
		rgb_write(12, 16, infoDisplay.coachName[4], 0b1, 2);
		rgb_write(24, 16, infoDisplay.coachName[5], 0b1, 2);
		xCoachLineEnd = 38;
	}
}

static void layoutTrainID(int16_t xOffset)
{
	uint8_t id_len = strlen(infoDisplay.trainInfo.id);
	int16_t xPos = xCoachLineEnd - xOffset;

	if (id_len)
	{
		if (infoDisplay.language == 0)
			xTrainIdLineEnd = rgb_print_constrain(xPos, 0, infoDisplay.trainInfo.id, id_len, 0b1, 2,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 0, 16) + xCoachLineEnd;
		else
			xTrainIdLineEnd = rgb_bangla_print_constrain(xPos, 0, infoDisplay.trainInfo.id, id_len,
					0b1, 1, xCoachLineEnd, MATRIX_MAX_WIDTH, 0, 16) + xCoachLineEnd;

		xTrainIdLineEnd += 10;
	}
	else
		xTrainIdLineEnd = xCoachLineEnd;
}

static int16_t layoutTrainName(int16_t xOffset)
{
	uint16_t train_len = strlen(infoDisplay.trainInfo.name);
	int16_t xPos = xTrainIdLineEnd - xOffset;
	int16_t xEnd = MATRIX_MAX_WIDTH;

	if (train_len)
	{
		if (infoDisplay.language == 0)
		{
#if DISPLAY_OUTDOOR
			rgb_print_constrain(xPos, 0, infoDisplay.trainInfo.name, train_len, 0b1, 2,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 0, 16);
#elif DISPLAY_INDOOR
			rgb_print_constrain(xPos, 0, infoDisplay.trainInfo.name, train_len, 0b1, 2,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 0, 16);
#endif	//if DISPLAY_OUTDOOR

			xEnd = (train_len * (5 * 2 + 2)) + xPos;
		}
		else
		{
#if DISPLAY_OUTDOOR
			rgb_bangla_print_constrain(xPos, 0, infoDisplay.trainInfo.name, train_len, 0b1, 1,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 0, 16);
#elif DISPLAY_INDOOR
			rgb_bangla_print_constrain(xPos, 0, infoDisplay.trainInfo.name, train_len, 0b1, 1,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 0, 16);
#endif	//if DISPLAY_OUTDOOR

			xEnd = train_len * 10 + xPos + 2;
		}
	}

	return xEnd;
}

/* TODO outdoor & indoor handler*/
#if DISPLAY_OUTDOOR
static int16_t layoutTrainRoute(int16_t xOffset)
{
	int16_t xPos = xCoachLineEnd;
	int16_t x = 0, x1;
	int16_t xEnd = MATRIX_MAX_WIDTH;
	int16_t i16a, i16b;

	i16a = strlen(infoDisplay.stationInfo.first);
	i16b = strlen(infoDisplay.stationInfo.end);

	if (i16a > 0 && i16b > 0)
	{
		if (infoDisplay.language == 0)
		{
			x = xPos - xOffset;
			x1 = rgb_print_constrain(x, 16, infoDisplay.stationInfo.first, i16a, 0b1, 2,
					xCoachLineEnd,
					MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);
			x += x1;

			x1 = rgb_print_constrain(x, 16, (char *) to_in_eng, strlen(to_in_eng), 0b1, 2,
					xCoachLineEnd,
					MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);
			x += x1;

			x1 = rgb_print_constrain(x, 16, infoDisplay.stationInfo.end, i16b, 0b1, 2,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);
			x += x1;
		}
		else
		{
			x = xPos - xOffset;
			x1 = rgb_bangla_print_constrain(x, 16, infoDisplay.stationInfo.first,
					strlen(infoDisplay.stationInfo.first), 0b1, 1, xPos, MATRIX_MAX_WIDTH, 16,
					MATRIX_MAX_HEIGHT);

			x += x1;
			x1 = rgb_bangla_print_constrain(x, 16, (char*) to_in_bangla, strlen(to_in_bangla), 0b1,
					1, xPos, MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);

			x += x1;
			x1 = rgb_bangla_print_constrain(x, 16, infoDisplay.stationInfo.end,
					strlen(infoDisplay.stationInfo.end), 0b1, 1, xPos, MATRIX_MAX_WIDTH, 16,
					MATRIX_MAX_HEIGHT);
			x += x1 + 2;
		}
		xEnd = x;
	}

	return xEnd;
}
#endif	//if DISPLAY_OUTDOOR

#if DISPLAY_INDOOR
static int16_t layoutTrainRoute(int16_t xOffset)
{
	int16_t x = 0, x1;
	int16_t xEnd = MATRIX_MAX_WIDTH;
	int16_t i16a;

	i16a = strlen(infoDisplay.stationInfo.name);

	if (i16a > 0)
	{
		if (infoDisplay.language == 0)
		{
			x = xCoachLineEnd - xOffset;
			if (infoDisplay.stationInfo.state == STATION_ARRIVED)
			x1 = rgb_print_constrain(x, 16, (char*) at_in_eng, strlen(at_in_eng), 0b1, 2,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);
			else
			x1 = rgb_print_constrain(x, 16, (char*) to_in_eng, strlen(to_in_eng), 0b1, 2,
					xCoachLineEnd, MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);
			x += x1;
			x1 = rgb_print_constrain(x, 16, infoDisplay.stationInfo.name,
					strlen(infoDisplay.stationInfo.name), 0b1, 2, xCoachLineEnd, MATRIX_MAX_WIDTH,
					16, MATRIX_MAX_HEIGHT);

			x += x1;
		}
		else
		{
			x = xCoachLineEnd - xOffset;
			if (infoDisplay.stationInfo.state == STATION_ARRIVED)
			x1 = rgb_bangla_print_constrain(x, 16, (char*) at_in_bangla, strlen(at_in_bangla),
					0b1, 1, xCoachLineEnd, MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);
			else
			x1 = rgb_bangla_print_constrain(x, 16, (char*) to_in_bangla, strlen(to_in_bangla),
					0b1, 1, xCoachLineEnd, MATRIX_MAX_WIDTH, 16, MATRIX_MAX_HEIGHT);
			x += x1;
			x1 = rgb_bangla_print_constrain(x, 16, infoDisplay.stationInfo.name,
					strlen(infoDisplay.stationInfo.name), 0b1, 1, xCoachLineEnd, MATRIX_MAX_WIDTH,
					16, MATRIX_MAX_HEIGHT);

			x += x1 + 2;
		}
		xEnd = x;
	}

	return xEnd;
}
#endif	//if DISPLAY_INDOOR

static void layoutConnectedValid(uint8_t coach_len)
{
	rgb_frame_clear();

	layoutCoachName(coach_len);
	layoutTrainID(0);
	layoutTrainName(0);
	layoutTrainRoute(0);

	swapBufferStart = 1;
}

static uint8_t layoutConnected(uint8_t *lang)
{
	uint8_t ret = 0;
	uint8_t u8a = 0, u8b = 0;
	uint8_t displayLang = *lang;

	if (displayLang)
	{
		infoDisplay = infoBangla;
		*lang = 0;
	}
	else
	{
		infoDisplay = infoEng;
		*lang = 1;
	}

	u8a = strlen(infoDisplay.coachName);
	if (!u8a)
	{
		if (displayLang == 0)
			u8b = strlen(infoBangla.coachName);
		else
			u8b = strlen(infoEng.coachName);

		if (!u8b)
			return 0;
		else
			return 1;
	}
	else
	{
		layoutConnectedValid(u8a);
		return 2;
	}

	return ret;
}

static void layoutDisconnected(uint8_t * lang)
{
	uint8_t displayLang = *lang;
	int16_t xPos = 0;

	rgb_frame_clear();
	if (displayLang)
	{
		/* english */
#if DISPLAY_OUTDOOR
		xPos = 36;
#endif	//if DISPLAY_OUTDOOR
#if DISPLAY_INDOOR
		xPos = 4;
#endif	//if DISPLAY_INDOOR

		rgb_print(xPos, 0, (char *) bangladesh_eng, strlen(bangladesh_eng), 0b1, 2);
		rgb_print(xPos + 12, 16, (char *) railways_eng, strlen(railways_eng), 0b1, 2);

		*lang = 0;
	}
	else
	{
		/* bangla */
#if DISPLAY_OUTDOOR
		xPos = 56;
#endif	//if DISPLAY_OUTDOOR
#if DISPLAY_INDOOR
		xPos = 24;
#endif	//if DISPLAY_INDOOR

		rgb_bangla_print(xPos, 0, (char *) bangladesh_bangla, strlen(bangladesh_bangla), 0b1, 1);
		rgb_bangla_print(xPos + 10, 16, (char *) railways_bangla, strlen(railways_bangla), 0b1, 1);

		*lang = 1;
	}
	swapBufferStart = 1;
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

static uint8_t parseCommand()
{
	uint8_t ret = 0;
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
		if (ring_buffer_readString(&uartBuffer, cmdIn, UART_BUFSIZE, CMD_CHAR_HEADER,
		CMD_CHAR_ENDLINE) == HAL_OK)
		{
			LED_BUILTIN_GPIO_Port->ODR ^= LED_BUILTIN_Pin;

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
				cmdTem.language = cmdCounter % 2;

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
				{
					parseBangla(&cmdTem);
					infoBangla = cmdTem;
				}
				else
				{
					infoEng = cmdTem;
				}

				ret = 1;
			}
		}
		uart1RxReady = RESET;
	}

	return ret;
}

static void parseStringBangla(char *str, uint16_t size)
{
	char lbuf[COMMAND_LONG_BUFSIZE];
	uint16_t len = 0;
	uint16_t indexChar = 0;
	uint16_t indexStr = 0;
	char buf[COMMAND_SHORT_BUFSIZE];
	char *tmp;
	uint8_t u8a, u8b;

	tmp = str;
	len = strlen(str);

	memset(lbuf, 0, COMMAND_LONG_BUFSIZE);
	memset(buf, 0, COMMAND_SHORT_BUFSIZE);
	indexChar = 0;
	for ( int i = 0; i < len; i++ )
	{
		if (*(tmp + i) == ':' || i == len - 1)
		{
			if (i == len - 1)
				buf[indexChar++] = *(tmp + i);

			u8a = strlen(buf);
			if (u8a > 0)
			{
				u8b = 0;
				if (u8a > 1)
				{
					u8b = char_to_byte(buf[0]) * 10;
					u8b += char_to_byte(buf[1]);
				}
				else
					u8b = char_to_byte(buf[0]);
				lbuf[indexStr++] = u8b;
			}
			memset(buf, 0, COMMAND_SHORT_BUFSIZE);
			indexChar = 0;
		}
		else
			buf[indexChar++] = *(tmp + i);
	}

	memcpy(str, lbuf, size);
}

static void parseBangla(CMD_HandleTypeDef *cmd)
{
	parseStringBangla(cmd->trainInfo.id, COMMAND_SHORT_BUFSIZE);
	parseStringBangla(cmd->trainInfo.name, COMMAND_LONG_BUFSIZE);
	parseStringBangla(cmd->stationInfo.name, COMMAND_LONG_BUFSIZE);
	parseStringBangla(cmd->stationInfo.first, COMMAND_LONG_BUFSIZE);
	parseStringBangla(cmd->stationInfo.end, COMMAND_LONG_BUFSIZE);
}

/* TODO */

static void startDmaTransfering()
{
	HAL_DMA_Start_IT(&hdma_tim1_up, activeFrameRowAdress, (uint32_t) &DATA_GPIO->ODR, FRAME_SIZE);
}

static void dmaTransferCompleted(DMA_HandleTypeDef *hdma)
{
	DATA_GPIO->BSRR = DATA_BSSR_CLEAR;

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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* set stb high */
	/* set matrix row */
	CONTROL_GPIO->ODR = (matrix_row << CONTROL_SHIFTER) | stb_Pin;
	startDmaTransfering();
}

static void ring_buffer_write(Ring_Buffer_t *buffer, char c)
{
	uint16_t i = (buffer->head + 1) % buffer->size;

	if (i != buffer->tail)
	{
		buffer->buffer[buffer->head] = c;
		buffer->head = i;
	}
}

static HAL_StatusTypeDef ring_buffer_read(Ring_Buffer_t *buffer, char *c)
{
	if (buffer->head != buffer->tail)
	{
		char recvByte = buffer->buffer[buffer->tail];
		buffer->tail = (buffer->tail + 1) % buffer->size;

		*c = recvByte;

		return HAL_OK;
	}
	return HAL_ERROR;
}

static HAL_StatusTypeDef ring_buffer_readString(Ring_Buffer_t *buffer, char *to, uint16_t size,
		char start, char end)
{
	char c;
	char * sendTo;
	uint16_t index = 0;
	FlagStatus startSaveBuffer = RESET;

	memset(to, 0, size);
	sendTo = to;
	while (buffer->head != buffer->tail)
	{
		ring_buffer_read(buffer, &c);
		if (c == start && startSaveBuffer == RESET)
			startSaveBuffer = SET;

		if (startSaveBuffer == SET)
		{
			*(sendTo + index) = c;
			if (c == end)
				break;

			if (++index >= size)
			{
				memset(to, 0, size);
				return HAL_ERROR;
			}
		}
	}
	return HAL_OK;
}

void usart1_callback_IT()
{
	uint32_t isrflags = READ_REG(huart1.Instance->SR);
	char c;

	/* RXNE handler */
	if ((isrflags & USART_SR_RXNE) != RESET)
	{
		c = (char) (huart1.Instance->DR & (uint8_t) 0x00FF);
		ring_buffer_write(&uartBuffer, c);
		if (c == CMD_CHAR_ENDLINE)
			uart1RxReady = SET;
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
