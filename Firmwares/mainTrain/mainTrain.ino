#include "Arduino.h"
#include "menuDisplay.h"

const byte LCD_BACKLIGHT_PIN = A7;
const byte LCD_SS_PIN = 48;
const byte SD_SS_PIN = 53;
//button= {RIGHT, LEFT, UP, DOWN}
const byte button[4] = { A3, A2, A0, A1 };
const uint32_t DEBOUNCE_DELAY = 100;

enum BUTTON_VALUE
{
	BUTTON_NONE, BUTTON_RIGHT, BUTTON_LEFT, BUTTON_UP, BUTTON_DOWN
};

enum DISPLAY_STATE
{
	DISP_IDLE,
	DISP_MAIN,
	DISP_MENU,
	DISP_MENU_TRAIN_NAME,
	DISP_MENU_TRAIN_ROUTE,
	DISP_MENU_CURRENT_STATION,
	DISP_MENU_DISPLAY_LANGUAGE,
	DISP_MENU_MENU_LANGUAGE,
	DISP_MENU_MASTER_MODE,
	DISP_MENU_GPS_MODE,
	DISP_MENU_SYSTEM_INFO,
	DISP_MENU_COACH,
	DISP_MENU_COACH_TYPE,
	DISP_MENU_COACH_NUMBER
};
byte displayState = DISP_IDLE;
String trainName[5];
String trainRoute[2];
String stationName[5];
enum MASTER_MODE
{
	SLAVE_MODE, MASTER_MODE
};
bool masterMode;
enum GPS_MODE
{
	MANUAL_MODE, GPS_MODE
};
bool gpsMode;
enum LANGUAGE
{
	ENGLISH_LANG, BANGLADESH_LANG, INDONESIAN_LANG
};
byte displayLanguage = ENGLISH_LANG;
byte menuLanguage = ENGLISH_LANG;

menuDisplay md(LCD_SS_PIN, LCD_BACKLIGHT_PIN);

void setup()
{
	buttonInit();
	lcdInit();
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
	md.init();
}

void lcdHandler()
{
	//	md.pageSubMenu("Train Name", "Train7", "Train1", "Train2", 2);
	md.pageSubMenu("Master/Slave Mode", "Master", "Slave", 1);
	delay(3000);
	//	md.pageSubMenu("Train Name", "Train6", "Train7", "Train1", 2);
	md.pageSubMenu("Master/Slave Mode", "Master", "Slave", 0);
	delay(3000);
}

/**
 * BUTTON
 */
void buttonInit()
{
	byte a;

	for ( a = 0; a < 4; a++ )
		pinMode(button[a], INPUT_PULLUP);
}

