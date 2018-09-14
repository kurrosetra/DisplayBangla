/*
 * matrix.c
 *
 *  Created on: Sep 3, 2018
 *      Author: miftakur
 */

#include "matrix.h"
#include "font5x7.h"
#include "fontBangla.h"

void rgb_frame_clear()
{
	for ( int row = 0; row < MATRIX_SCANROW; row++ )
	{
		memset(frameBuffer[row], (uint8_t) clk_en_Pin, FRAME_SIZE);
		for ( int i = FRAME_START_OFFSET + FRAME_BUFSIZE; i < FRAME_SIZE; i++ )
			frameBuffer[row][i] = 0;
	}
}

void rgb_draw_pixel(int16_t x, int16_t y, uint8_t color)
{
	uint8_t busNumber = 0;
	uint16_t row, lines;
	uint8_t rgbVal = 0;
#if MATRIX_SCANROW==MATRIX_SCANROWS_8
	uint16_t y_segment = 0;
#endif	//if MATRIX_SCANROW==MATRIX_SCANROWS_8

#if FRAME_ORIENTATION==FRAME_ORIENTATION_3
	x = MATRIX_MAX_WIDTH - 1 - x;
	y = MATRIX_MAX_HEIGHT - 1 - y;
#endif	//if FRAME_ORIENTATION==FRAME_ORIENTATION_3

#if (FRAME_ORIENTATION==FRAME_ORIENTATION_2)||(FRAME_ORIENTATION==FRAME_ORIENTATION_4)
	int16_t xy=x;

	x=y;
	y=xy;
#if FRAME_ORIENTATION==FRAME_ORIENTATION_4
	x = MATRIX_MAX_WIDTH - x;
	y = MATRIX_MAX_HEIGHT - y;
#endif	//elif FRAME_ORIENTATION==FRAME_ORIENTATION_4
#endif	//if (FRAME_ORIENTATION==FRAME_ORIENTATION_2)||(FRAME_ORIENTATION==FRAME_ORIENTATION_4)

	if ((x >= 0 && x < MATRIX_MAX_WIDTH) && (y >= 0 && y < MATRIX_MAX_HEIGHT))
	{
#if MATRIX_SCANROW==MATRIX_SCANROWS_8
		y_segment = y;
		if (y >= MATRIX_PANEL_HEIGHT / 2)
		{
			busNumber = 8;
			y_segment = y - 16;
		}

		if (y_segment >= 8)
		{
			row = y_segment - 8;
			lines = (x / 8) * 16 + x % 8;
		}
		else
		{
			row = y_segment;
			lines = (x / 8) * 16 + (15 - x % 8);
		}

#endif	//if MATRIX_SCANROW==MATRIX_SCANROWS_8
#if MATRIX_SCANROW==MATRIX_SCANROWS_16
		row = y % 16;
		lines = x;
#endif	//if MATRIX_SCANROW==MATRIX_SCANROWS_16

		/* TODO handle color*/
		if (color & 0b1)
		{
			if (busNumber == 0)
				rgbVal = 0b1;
			else
				rgbVal = 0b1000;
			frameBuffer[row][lines] |= rgbVal;
		}
		if (color & 0b10)
		{
			if (busNumber == 0)
				rgbVal = 0b10;
			else
				rgbVal = 0b10000;
			frameBuffer[row][lines] |= rgbVal;

		}
		if (color & 0b100)
		{
			if (busNumber == 0)
				rgbVal = 0b100;
			else
				rgbVal = 0b100000;
			frameBuffer[row][lines] |= rgbVal;
		}

	}
}

void rgb_write_constrain(int16_t x, int16_t y, char c, uint8_t color, uint8_t fontSize,
		int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax)
{
	uint8_t getFont[5];
	uint16_t fontPos = (uint16_t) c;
	uint16_t xPos, yPos;
	uint16_t xBit, yBit;
	uint16_t xSize, ySize;

	for ( xPos = 0; xPos < FONT_X_SIZE; xPos++ )
		getFont[xPos] = *(font5x7 + (fontPos * FONT_X_SIZE) + xPos);

	for ( xPos = 0; xPos < FONT_X_SIZE; xPos++ )
	{
		for ( yPos = 0; yPos < FONT_Y_SIZE; yPos++ )
		{
			if (bitRead(getFont[xPos], yPos))
			{
				for ( xSize = 0; xSize < fontSize; xSize++ )
				{
					for ( ySize = 0; ySize < fontSize; ySize++ )
					{
						yBit = y + (yPos * fontSize) + ySize;
						xBit = x + (xPos * fontSize) + xSize;
						if ((xBit >= xMin && xBit <= xMax) && (yBit >= yMin && yBit <= yMax))
						{
							rgb_draw_pixel(xBit, yBit, color);
						}
					}
				}
			}
		}
	}

#if USE_SMOOTH_FONT
	/* smoothing font when size>1 */
	if (fontSize > 1)
	{
		for ( xPos = 0; xPos < FONT_X_SIZE; xPos++ )
		{
			if (xPos == 1 || xPos == 3)
			{
				for ( yPos = 0; yPos < FONT_Y_SIZE; yPos++ )
				{
					if (bitRead(getFont[xPos], yPos))
					{
						//left
						if (!bitRead(getFont[xPos - 1], yPos))
						{
							//left-down
							if (bitRead(getFont[xPos - 1],
									yPos - 1) && !bitRead(getFont[xPos], yPos - 1))
							{
								for ( int i = 0; i < fontSize; i++ )
								{
									for ( xSize = 0; xSize < fontSize; xSize++ )
									{
										for ( ySize = 0; ySize < fontSize; ySize++ )
										{
											yBit = y + (yPos * fontSize) + ySize - i;
											xBit = x + (xPos * fontSize) + xSize - i;
											if ((xBit >= xMin && xBit <= xMax)
													&& (yBit >= yMin && yBit <= yMax))
											{
												rgb_draw_pixel(xBit, yBit, color);
											}
										}
									}
								}

							}
							//left-up
							if (bitRead(getFont[xPos - 1],
									yPos + 1) && !bitRead(getFont[xPos], yPos + 1))
							{
								for ( int i = 0; i < fontSize; i++ )
								{
									for ( xSize = 0; xSize < fontSize; xSize++ )
									{
										for ( ySize = 0; ySize < fontSize; ySize++ )
										{
											yBit = y + (yPos * fontSize) + ySize + i;
											xBit = x + (xPos * fontSize) + xSize - i;
											if ((xBit >= xMin && xBit <= xMax)
													&& (yBit >= yMin && yBit <= yMax))
											{
												rgb_draw_pixel(xBit, yBit, color);
											}
										}
									}
								}
							}
						}
						//right
						if (!bitRead(getFont[xPos + 1], yPos))
						{
							//right-down
							if (bitRead(getFont[xPos+1],yPos-1) && !bitRead(getFont[xPos], yPos - 1))
							{
								for ( int i = 0; i < fontSize; i++ )
								{
									for ( xSize = 0; xSize < fontSize; xSize++ )
									{
										for ( ySize = 0; ySize < fontSize; ySize++ )
										{
											yBit = y + (yPos * fontSize) + ySize - i;
											xBit = x + (xPos * fontSize) + xSize + i;
											if ((xBit >= xMin && xBit <= xMax)
													&& (yBit >= yMin && yBit <= yMax))
											{
												rgb_draw_pixel(xBit, yBit, color);
											}
										}
									}
								}
							}
							//right-up
							if (bitRead(getFont[xPos + 1],
									yPos + 1) && !bitRead(getFont[xPos], yPos + 1))
							{
								for ( int i = 0; i < fontSize; i++ )
								{
									for ( xSize = 0; xSize < fontSize; xSize++ )
									{
										for ( ySize = 0; ySize < fontSize; ySize++ )
										{
											yBit = y + (yPos * fontSize) + ySize + i;
											xBit = x + (xPos * fontSize) + xSize + i;
											if ((xBit >= xMin && xBit <= xMax)
													&& (yBit >= yMin && yBit <= yMax))
											{
												rgb_draw_pixel(xBit, yBit, color);
											}
										}
									}
								}

							}
						}
					}
				}
			}
		}
	}  // smoothing
#endif	//if USE_SMOOTH_FONT

}

void rgb_write(int16_t x, int16_t y, char c, uint8_t color, uint8_t fontSize)
{
	rgb_write_constrain(x, y, c, color, fontSize, 0, MATRIX_MAX_WIDTH - 1, 0,
	MATRIX_MAX_HEIGHT - 1);
}

void rgb_print(int16_t x, int16_t y, char* s, uint16_t size, uint8_t color, uint8_t fontSize)
{
	for ( uint16_t i = 0; i < size; i++ )
		rgb_write(x + (i * (FONT_X_SIZE * fontSize + fontSize)), y, *(s + i), color, fontSize);
}
void rgb_print_constrain(int16_t x, int16_t y, char* s, uint16_t size, uint8_t color,
		uint8_t fontSize, int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax)
{
	for ( uint16_t i = 0; i < size; i++ )
		rgb_write_constrain(x + (i * (FONT_X_SIZE * fontSize + fontSize)), y, *(s + i), color,
				fontSize, xMin, xMax, yMin, yMax);

}

void rgb_bangla_write_constrain(int16_t x, int16_t y, char c, uint8_t color, uint8_t fontSize,
		int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax)
{
	uint16_t line = 0;
	uint16_t fontPos = (uint16_t) c;
	int16_t xPos, yPos;

	for ( int i = 0; i < FONT_BANGLA_X_SIZE; i++ )
	{
		line = *(fontBangla + (fontPos * FONT_BANGLA_X_SIZE) + i);

		for ( int j = 0; j < 16; j++ )
		{
			if (line & 0x1)
			{
				xPos = x + i;
				yPos = y + j;
//				rgb_draw_pixel(xPos, yPos, color);
				if ((xPos >= xMin && xPos < xMax) && (yPos >= yMin && yPos < yMax))
					rgb_draw_pixel(xPos, yPos, color);
			}
			line >>= 1;
		}
	}
}

void rgb_bangla_write(int16_t x, int16_t y, char c, uint8_t color, uint8_t fontSize)
{
	rgb_bangla_write_constrain(x, y, c, color, fontSize, 0, MATRIX_MAX_WIDTH, 0,
	MATRIX_MAX_HEIGHT);
}

void rgb_bangla_print(int16_t x, int16_t y, char* s, uint16_t size, uint8_t color, uint8_t fontSize)
{
	for ( uint16_t i = 0; i < size; i++ )
		rgb_bangla_write(x + (i * (FONT_BANGLA_X_SIZE * fontSize)), y, *(s + i), color, fontSize);

}

void rgb_bangla_print_constrain(int16_t x, int16_t y, char* s, uint16_t size, uint8_t color,
		uint8_t fontSize, int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax)
{
	for ( uint16_t i = 0; i < size; i++ )
		rgb_bangla_write_constrain(x + (i * (FONT_BANGLA_X_SIZE * fontSize)), y, *(s + i), color,
				fontSize, xMin, xMax, yMin, yMax);

}
