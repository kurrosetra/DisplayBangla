#include "Arduino.h"
#include <SD.h>
#include "menuDisplay.h"

/**
 * DECLARATION
 */
#define DEBUG			1
#if DEBUG
# define LCD_DEBUG		1
# define SD_DEBUG		1
# define BUTTON_DEBUG	1
#endif	//#if DEBUG

/**
 * PINOUT
 */
const byte LCD_BACKLIGHT_PIN = A7;
const byte LCD_SS_PIN = 48;
const byte SD_SS_PIN = 53;
//button= {RIGHT, LEFT, UP, DOWN}
const byte button[4] = { A3, A2, A0, A1 };
const uint32_t DEBOUNCE_DELAY = 100;

/**
 * Global variables
 */
enum BUTTON_VALUE
{
	BUTTON_NONE,
	BUTTON_RIGHT,
	BUTTON_LEFT,
	BUTTON_UP,
	BUTTON_DOWN
};

File root;
menuDisplay md(LCD_SS_PIN, LCD_BACKLIGHT_PIN);

void setup()
{
#if DEBUG
	Serial.begin(115200);
	Serial.println(F("Display Bangla - INKA firmware"));
#endif	//#if DEBUG

	pinMode(LCD_SS_PIN, OUTPUT);
	digitalWrite(LCD_SS_PIN, HIGH);
	pinMode(SD_SS_PIN, OUTPUT);
	digitalWrite(SD_SS_PIN, HIGH);

	buttonInit();
	lcdInit();
	sdInit();
}

void loop()
{
	lcdHandler();
}

/**
 * LCD
 */
void lcdInit()
{
#if LCD_DEBUG
	Serial.print(F("lcd init ..."));
#endif	//#if LCD_DEBUG

	pinMode(SD_SS_PIN, HIGH);
	md.init();
	md.pageUpdate();

#if LCD_DEBUG
	Serial.println(F("done!"));
#endif	//#if LCD_DEBUG

}

void lcdHandler()
{

}

/**
 * SD CARD
 */
void sdInit()
{
#if SD_DEBUG
	Serial.println(F("SD Card init ..."));
#endif	//#if SD_DEBUG

	digitalWrite(LCD_SS_PIN, HIGH);
	if (!SD.begin(SD_SS_PIN)) {
#if SD_DEBUG
		Serial.println(F("initialization failed!"));
#endif	//#if SD_DEBUG
	}	//(!SD.begin(SD_SS_PIN))

#if SD_DEBUG
	Serial.println(F("done!"));
#endif	//#if SD_DEBUG

}

/**
 * BUTTON
 */
void buttonInit()
{
	byte a;

#if BUTTON_DEBUG
	Serial.print(F("Button init..."));
#endif	//#if BUTTON_DEBUG

	for ( a = 0; a < 4; a++ )
		pinMode(button[a], INPUT_PULLUP);

#if BUTTON_DEBUG
	Serial.println(F("done!"));
#endif	//#if BUTTON_DEBUG

}

