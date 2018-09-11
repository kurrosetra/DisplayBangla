/*
 * matrix.h
 *
 *  Created on: Sep 3, 2018
 *      Author: miftakur
 */

#ifndef MATRIX_H_
#define MATRIX_H_

#include "config.h"
#include <string.h>

#define USE_SMOOTH_FONT			1

uint8_t frameBuffer[MATRIX_SCANROW][FRAME_SIZE];

void rgb_frame_clear();
void rgb_draw_pixel(int16_t x, int16_t y, uint8_t color);
void rgb_write_constraint(int16_t x, int16_t y, char c, uint8_t color, uint8_t size, int16_t xMin,
		int16_t xMax, int16_t yMin, int16_t yMax);
void rgb_write(int16_t x, int16_t y, char c, uint8_t color, uint8_t size);
void rgb_print(int16_t x, int16_t y, char* s, uint8_t color, uint8_t size);

void rgb_bangla_write_constraint(int16_t x, int16_t y, char c, uint8_t color, uint8_t size, int16_t xMin,
		int16_t xMax, int16_t yMin, int16_t yMax);
void rgb_bangla_write(int16_t x, int16_t y, char c, uint8_t color, uint8_t size);
void rgb_bangla_print(int16_t x, int16_t y, char* s, uint8_t color, uint8_t size);

#endif /* MATRIX_H_ */
