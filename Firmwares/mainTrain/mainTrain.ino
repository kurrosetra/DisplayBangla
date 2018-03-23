#include "Arduino.h"
#include <SPI.h>
#include <SD.h>
#include "U8glib.h"

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
const byte button[4] = { A3/*RIGHT / SELECT*/, A2/*LEFT / BACK*/,
		A0/*UP / PREV*/, A1 /*DOWN / NEXT*/};
const uint32_t DEBOUNCE_DELAY = 100;

/**
 * *******************************************************************************
 * LCD Global variables
 * *******************************************************************************
 */

enum DISPLAY_STEP
{
	DISP_STEP_NONE,
	DISP_STEP_NEXT,
	DISP_STEP_PREV,
	DISP_STEP_SELECT,
	DISP_STEP_BACK
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
	DISP_MENU_COACH,
	DISP_MENU_SYSTEM_INFO,
	DISP_MENU_COACH_NUMBER,
	DISP_MENU_CHANGE_REQ
};
enum MASTER_MODE
{
	SLAVE_MODE,
	MASTER_MODE
};

enum GPS_MODE
{
	MANUAL_MODE,
	GPS_MODE
};

enum LANGUAGE
{
	ENGLISH_LANG,
	BANGLADESH_LANG
};

struct Coach_List
{
	char *name;
	byte number;
};

struct Train_Info
{
	uint16_t trainID;	// train's position in train.txt
	char * trainName;	// train's name
	char * trainNum;	// train's number
	char * currentStation;	// current station's name
	uint16_t currentStationPosition;  // position of current station in "default.PID"
	bool trainRoute;	// train's route (down=0, up=1)
};

struct Train_Detail
{
	byte menuLang;		// menu language (0=en, 1=ba)
	byte dispLang;		// display language (0=en, 1=ba)
	Train_Info trainInfo;		// current train's info
	Coach_List currentCoach;	// current coach's info
	bool masterMode;	// master/slave mode (slave=0, master=1)
	bool gpsMode;		// gps/manual mode (manual=0, gps=1)
};
static const byte menu_setting_length = 9;
const char *menu_setting_en[menu_setting_length] = { "Train Name",
		"Train Route", "Current Station", "Display Language", "Menu Language",
		"Master / Slave Mode", "Manual / GPS Mode", "Coach ID", "System Info" };
String menu_setting_bl[menu_setting_length] = {

};
byte displayState = DISP_MAIN;
Train_Detail trainDetail;
byte currentDisplayState = DISP_IDLE;
byte menuPointer = 0;
U8GLIB_ST7920_128X64_4X lcd(LCD_SS_PIN);

/**
 * *******************************************************************************
 * BUTTON Global Variables
 * *******************************************************************************
 */
enum BUTTON_VALUE
{
	BUTTON_NONE = 0,
	BUTTON_RIGHT = 0b1,
	BUTTON_LEFT = 0b10,
	BUTTON_UP = 0b100,
	BUTTON_DOWN = 0b1000
};

/**
 * *******************************************************************************
 * SD Card Global variables
 * *******************************************************************************
 */
File root;
File myFile;

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

	sdInit();
	btnInit();
	lcdInit();
}

void loop()
{
	btnHandler();
}

/**
 * *******************************************************************************
 * LCD functions
 * *******************************************************************************
 */
void lcdInit()
{
	pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
	lcdBacklight(1);
}

void lcdBacklight(bool onf)
{
	digitalWrite(LCD_BACKLIGHT_PIN, onf);
}

void lcdPageUpdate(byte dispStep)
{
	switch (displayState)
	{
	case DISP_IDLE:
		if (dispStep != DISP_STEP_NONE) {
			//change display to main page with backlight ON
			lcdBacklight(HIGH);
//			menuChange(pageMain);
			displayState = DISP_MAIN;
		}	//(dispStep != DISP_STEP_NONE)
		break;
	case DISP_MAIN:
		switch (dispStep)
		{
		case DISP_STEP_SELECT:
			//goto menu page
			menuPointer = 0;
			lcdPageChange (lcdPageMenu);
			displayState = DISP_MENU;
			break;
		case DISP_STEP_BACK:
			//goto idle page
			lcdBacklight(LOW);
			displayState = DISP_IDLE;
			break;
		}
		break;
	case DISP_MENU:
		switch (dispStep)
		{
		case DISP_STEP_NEXT:
			menuPointer++;
			if (menuPointer >= menu_setting_length)
				menuPointer = 0;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_PREV:
			if (menuPointer == 0)
				menuPointer = menu_setting_length - 1;
			lcdPageChange(lcdPageMenu);
			break;
		case DISP_STEP_BACK:
			//back to DISP_MAIN state
			menuPointer = 0;
			lcdPageChange (lcdPageMain);
			break;
		case DISP_STEP_SELECT:
			displayState = menuPointer + 3;
			lcdPageChange (lcdPageSubMenu);
			break;
		}
		break;
	}
}

void lcdPageChange(void (*updatePage)())
{
	digitalWrite(SD_SS_PIN, HIGH);
	lcd.firstPage();
	do {
		(*updatePage)();
	} while (lcd.nextPage());
	digitalWrite(LCD_SS_PIN, HIGH);
}

void lcdPageMain()
{
	String s;
	u8g_uint_t w, d;

	//HEADER
	w = lcd.getWidth();
	lcd.setFont(u8g_font_ncenB08);
	lcd.setFontRefHeightText();
	lcd.setFontPosTop();
	if (trainDetail.menuLang == ENGLISH_LANG)
		s = F("Bangladesh Railway");
	else
		s = F("~Bangladesh Railway~");
	d = (w - lcd.getStrWidth(s.c_str())) / 2;
	lcd.drawStr(d, 0, s.c_str());

	//SUB-HEADER
	lcd.setFont(u8g_font_5x7r);
	if (trainDetail.menuLang == ENGLISH_LANG)
		s = F("Display Controller");
	else
		s = F("~Display Controller~");
	d = (w - lcd.getStrWidth(s.c_str())) / 2;
	lcd.drawStr(d, 25, s.c_str());
	//KONTEN 1
	//KONTEN 2
	if (trainDetail.masterMode) {
		if (trainDetail.menuLang == ENGLISH_LANG)
			s = F("Master");
		else
			s = F("~Master~");
	}
	else {
		if (trainDetail.menuLang == ENGLISH_LANG)
			s = F("Slave");
		else
			s = F("~Slave~");
	}
	if (trainDetail.menuLang == ENGLISH_LANG)
		s += ';';
	else
		s += "~;~";
	if (trainDetail.gpsMode) {
		if (trainDetail.menuLang == ENGLISH_LANG)
			s += F("GPS");
		else
			s += F("~GPS~");
	}
	else {
		if (trainDetail.menuLang == ENGLISH_LANG)
			s += F("Manual");
		else
			s += F("~Manual~");
	}
	d = (w - lcd.getStrWidth(s.c_str())) / 2;
	lcd.drawStr(d, 63, s.c_str());
}

void lcdPageMenu()
{
	String title, s[3];

	//HEADER
	lcd.setFont(u8g_font_ncenB08);
	lcd.setFontRefHeightText();
	lcd.setFontPosTop();
	if (trainDetail.menuLang == ENGLISH_LANG)
		title = F("Select Menu:");
	else
		title = F("~Select Menu~");
	lcd.drawStr(0, 0, title.c_str());

	//MENU CONTENT
	if (trainDetail.menuLang == ENGLISH_LANG)
		s[1] = menu_setting_en[menuPointer];
	else
		s[1] = menu_setting_bl[menuPointer];

	if (menuPointer == 0) {
		if (trainDetail.menuLang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menu_setting_length - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		}
		else {
			s[0] = menu_setting_bl[menu_setting_length - 1];
			s[2] = menu_setting_bl[menuPointer + 1];
		}
	}
	else if (menuPointer >= menu_setting_length - 1) {
		if (trainDetail.menuLang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[0];
		}
		else {
			s[0] = menu_setting_bl[menuPointer - 1];
			s[2] = menu_setting_bl[0];
		}
	}

	lcd.setFont(u8g_font_5x7);
	lcd.drawStr(0, 30, s[0].c_str());
	lcd.setDefaultForegroundColor();
	lcd.drawBox(0, 37, lcd.getStrWidth(s[1].c_str()) + 1, 10);
	lcd.setDefaultBackgroundColor();
	lcd.drawStr(1, 45, s[1].c_str());
	lcd.setDefaultForegroundColor();
	lcd.drawStr(2, 60, s[2].c_str());
}

void lcdPageSubMenu()
{
	String title, s[3];

	//HEADER
	lcd.setFont(u8g_font_ncenB08);
	lcd.setFontRefHeightText();
	lcd.setFontPosTop();
	if (trainDetail.menuLang == ENGLISH_LANG)
		title = menu_setting_en[menuPointer];
	else
		title = menu_setting_bl[menuPointer];
	lcd.drawStr(0, 0, title.c_str());

	//MENU CONTENT
	if (trainDetail.menuLang == ENGLISH_LANG)
		s[1] = menu_setting_en[menuPointer];
	else
		s[1] = menu_setting_bl[menuPointer];

	if (menuPointer == 0) {
		if (trainDetail.menuLang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menu_setting_length - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		}
		else {
			s[0] = menu_setting_bl[menu_setting_length - 1];
			s[2] = menu_setting_bl[menuPointer + 1];
		}
	}
	else if (menuPointer >= menu_setting_length - 1) {
		if (trainDetail.menuLang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[0];
		}
		else {
			s[0] = menu_setting_bl[menuPointer - 1];
			s[2] = menu_setting_bl[0];
		}
	}

	lcd.setFont(u8g_font_5x7);
	lcd.drawStr(0, 30, s[0].c_str());
	lcd.setDefaultForegroundColor();
	lcd.drawBox(0, 37, lcd.getStrWidth(s[1].c_str()) + 1, 10);
	lcd.setDefaultBackgroundColor();
	lcd.drawStr(1, 45, s[1].c_str());
	lcd.setDefaultForegroundColor();
	lcd.drawStr(2, 60, s[2].c_str());
}

/**
 * *******************************************************************************
 * SD CARD functions
 * *******************************************************************************
 */

void sdInit()
{
#if SD_DEBUG
	Serial.print(F("SD card init ..."));
#endif	//#if SD_DEBUG

	if (!SD.begin(SD_SS_PIN)) {
#if SD_DEBUG
		Serial.println(F("failed!"));
#endif	//#if SD_DEBUG

		sdFailedHandler();
	}	//(!SD.begin(SD_SS_PIN))
#if SD_DEBUG
	else
		Serial.println(F("done!"));
#endif	//#if SD_DEBUG
}

void sdFailedHandler()
{
}

/**
 * *******************************************************************************
 * Button functions
 * *******************************************************************************
 */

void btnInit()
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

void btnHandler()
{
	static uint32_t _update = 0;
	static byte _prevButtonVal = 0;
	byte buttonVal = 0;

	if (_update == 0)
		buttonVal = btnRead();

	if (buttonVal != _prevButtonVal) {
		if (_update == 0) {
			_update = millis() + DEBOUNCE_DELAY;
		}	//(_update == 0)
		else {
			if (millis() >= _update) {
				buttonVal = btnRead();
				if (buttonVal != _prevButtonVal) {
					_prevButtonVal = buttonVal;

					btnUpdate(buttonVal);
				}	//(buttonVal != _prevButtonVal)
				_update = 0;
			}	//(millis() >= _update)
		}
	}	//(buttonVal != _prevButtonVal)
}

void btnUpdate(byte button)
{
	if ((button == DISP_STEP_SELECT) || (button == DISP_STEP_BACK)
			|| (button == DISP_STEP_NEXT) || (button == DISP_STEP_PREV))
		lcdPageUpdate(button);
}

byte btnRead()
{
	byte ret = 0, a;

	for ( a = 0; a < 4; a++ )
		bitWrite(ret, a, !digitalRead(button[a]));

	return ret;
}
