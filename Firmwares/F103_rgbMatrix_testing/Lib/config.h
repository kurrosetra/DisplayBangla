/*
 * config.h
 *
 *  Created on: Sep 3, 2018
 *      Author: miftakur
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "stm32f1xx.h"

#define DISPLAY_OUTDOOR				1
#if DISPLAY_OUTDOOR==0
#define DISPLAY_INDOOR				1
#endif	//if DISPLAY_OUTDOOR==0

#define FRAME_ORIENTATION_1			1
#define FRAME_ORIENTATION_2			2
#define FRAME_ORIENTATION_3			3
#define FRAME_ORIENTATION_4			4

#define MATRIX_SCANROWS_8			8
#define MATRIX_SCANROWS_16			16

#if DISPLAY_OUTDOOR==1
#define FRAME_ORIENTATION			FRAME_ORIENTATION_1
#define MATRIX_SCANROW				MATRIX_SCANROWS_8

#elif DISPLAY_INDOOR==1
#define FRAME_ORIENTATION			FRAME_ORIENTATION_1
#define MATRIX_SCANROW				MATRIX_SCANROWS_16

#endif	//if DISPLAY_OUTDOOR==1

#if MATRIX_SCANROW==MATRIX_SCANROWS_8
#define MATRIX_PANEL_W_NUM			1
#elif MATRIX_SCANROW==MATRIX_SCANROWS_16
#define MATRIX_PANEL_W_NUM			2
#endif	//if MATRIX_SCANROW==MATRIX_SCANROWS_8
#define MATRIX_PANEL_WIDTH			64
#define MATRIX_PANEL_HEIGHT			32

#define MATRIX_MAX_WIDTH			(MATRIX_PANEL_WIDTH * MATRIX_PANEL_W_NUM)
#define MATRIX_MAX_HEIGHT			MATRIX_PANEL_HEIGHT

#define DATA_GPIO					GPIOA
#define CONTROL_GPIO				GPIOB
#define CONTROL_SHIFTER				0
#define CONTROL_BSSR_STB_CLEAR		0x00100000
#define DATA_BSSR_CLEAR				0x00FF0000

#define T2CCR1_MIN					((TIM2_MAX_DUTY*1)/100)
#if MATRIX_SCANROW==MATRIX_SCANROWS_8
#define FRAME_BUFSIZE				(MATRIX_MAX_WIDTH * 2)
#elif MATRIX_SCANROW==MATRIX_SCANROWS_16
#define FRAME_BUFSIZE				MATRIX_MAX_WIDTH
#endif	//if MATRIX_SCANROW==MATRIX_SCANROWS_8

#define FRAME_SIZE					(FRAME_BUFSIZE * 2)

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#endif /* CONFIG_H_ */
