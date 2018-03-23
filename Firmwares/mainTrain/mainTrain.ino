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
# define BUTTON_DEBUG	0
#endif	//#if DEBUG

/**
 * PINOUT
 */
const byte LCD_BACKLIGHT_PIN = A7;
const byte LCD_SS_PIN = 48;
const byte SD_SS_PIN = 53;
const byte button[4] = { A1/*RIGHT / SELECT*/, A3/*LEFT / BACK*/, A0 /*DOWN / NEXT*/, A2 /*UP / PREV*/};
const uint32_t DEBOUNCE_DELAY = 100;

/**
 * *******************************************************************************
 * LCD Global variables
 * *******************************************************************************
 */

enum DISPLAY_STEP
{
	DISP_STEP_NONE = 0,
	DISP_STEP_SELECT = 0b1,
	DISP_STEP_BACK = 0b10,
	DISP_STEP_NEXT = 0b100,
	DISP_STEP_PREV = 0b1000
};

enum DISPLAY_STATE
{
	DISP_IDLE,
	DISP_MAIN,
	DISP_MENU,
	DISP_MENU_TRAIN_NAME,
	DISP_MENU_TRAIN_ROUTE,
	DISP_MENU_CURRENT_STATION,
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
	String trainFiles;	// filename of selected's train
	String trainName;	// train's name
	String trainNum;	// train's number
	String currentStation;	// current station's name
	uint16_t currentStationPosition;  // position of current station in "default.PID"
	String routeStart;	// first station name
	String routeEnd;	// last station name
	bool trainRoute;	// train's route (down=0, up=1)
};

struct Train_Detail
{
	byte lang;		// menu language (0=en, 1=ba)
	Train_Info trainInfo;		// current train's info
	Coach_List currentCoach;	// current coach's info
	bool masterMode;	// master/slave mode (slave=0, master=1)
	bool gpsMode;		// gps/manual mode (manual=0, gps=1)
};
static const byte menu_setting_length = 8;
const char *menu_setting_en[menu_setting_length] = { "Train Name", "Train Route", "Current Station", "Language",
		"Master / Slave Mode", "Manual / GPS Mode", "Coach ID", "System Info" };
const String menu_setting_bl[menu_setting_length] = {

};
static const byte menu_coach_list_length = 20;
String menu_coach_list[menu_coach_list_length];

byte displayState = DISP_IDLE;
Train_Detail trainDetail;
byte currentDisplayState = DISP_IDLE;
byte menuPointer = 0;
U8GLIB_ST7920_128X64_4X lcd(LCD_SS_PIN);

/**
 * *******************************************************************************
 * BUTTON Global Variables
 * *******************************************************************************
 */

/**
 * *******************************************************************************
 * SD Card Global variables
 * *******************************************************************************
 */

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

	lcdPageUpdate(DISP_STEP_NEXT);

	Serial.println(F("no error"));
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
	lcdBacklight(0);
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
			lcdPageChange (lcdPageMain);
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
			else
				menuPointer--;
			lcdPageChange(lcdPageMenu);
			break;
		case DISP_STEP_BACK:
			//back to DISP_MAIN state
			menuPointer = 0;
			lcdPageChange (lcdPageMain);
			displayState = DISP_MAIN;
			break;
		case DISP_STEP_SELECT:
//			displayState = menuPointer + 3;
//			lcdPageChange (lcdPageSubMenu);
//			displayState=
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
	if (trainDetail.lang == ENGLISH_LANG)
		s = F("Bangladesh Railway");
	else
		s = F("~Bangladesh Railway~");
	d = (w - lcd.getStrWidth(s.c_str())) / 2;
	lcd.drawStr(d, 0, s.c_str());

	//SUB-HEADER
	lcd.setFont(u8g_font_5x7r);
	s = trainDetail.trainInfo.trainName;
	d = (w - lcd.getStrWidth(s.c_str())) / 2;
	lcd.drawStr(d, 25, s.c_str());
	//KONTEN 1
	//KONTEN 2
	if (trainDetail.masterMode) {
		if (trainDetail.lang == ENGLISH_LANG)
			s = F("Master");
		else
			s = F("~Master~");
	}
	else {
		if (trainDetail.lang == ENGLISH_LANG)
			s = F("Slave");
		else
			s = F("~Slave~");
	}
	if (trainDetail.lang == ENGLISH_LANG)
		s += ';';
	else
		s += "~;~";
	if (trainDetail.gpsMode) {
		if (trainDetail.lang == ENGLISH_LANG)
			s += F("GPS");
		else
			s += F("~GPS~");
	}
	else {
		if (trainDetail.lang == ENGLISH_LANG)
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
	if (trainDetail.lang == ENGLISH_LANG)
		title = F("Select Menu:");
	else
		title = F("~Select Menu~");
	lcd.drawStr(0, 0, title.c_str());

	//MENU CONTENT
	if (trainDetail.lang == ENGLISH_LANG)
		s[1] = menu_setting_en[menuPointer];
	else
		s[1] = menu_setting_bl[menuPointer];

	if (menuPointer == 0) {
		if (trainDetail.lang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menu_setting_length - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		}
		else {
			s[0] = menu_setting_bl[menu_setting_length - 1];
			s[2] = menu_setting_bl[menuPointer + 1];
		}
	}
	else if (menuPointer >= menu_setting_length - 1) {
		if (trainDetail.lang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[0];
		}
		else {
			s[0] = menu_setting_bl[menuPointer - 1];
			s[2] = menu_setting_bl[0];
		}
	}
	else {
		if (trainDetail.lang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		}
		else {
			s[0] = menu_setting_bl[menuPointer - 1];
			s[2] = menu_setting_bl[menuPointer + 1];
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
	if (trainDetail.lang == ENGLISH_LANG)
		title = menu_setting_en[menuPointer];
	else
		title = menu_setting_bl[menuPointer];
	lcd.drawStr(0, 0, title.c_str());

	//MENU CONTENT
	if (trainDetail.lang == ENGLISH_LANG)
		s[1] = menu_setting_en[menuPointer];
	else
		s[1] = menu_setting_bl[menuPointer];

	if (menuPointer == 0) {
		if (trainDetail.lang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menu_setting_length - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		}
		else {
			s[0] = menu_setting_bl[menu_setting_length - 1];
			s[2] = menu_setting_bl[menuPointer + 1];
		}
	}
	else if (menuPointer >= menu_setting_length - 1) {
		if (trainDetail.lang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[0];
		}
		else {
			s[0] = menu_setting_bl[menuPointer - 1];
			s[2] = menu_setting_bl[0];
		}
	}
	else {
		if (trainDetail.lang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		}
		else {
			s[0] = menu_setting_bl[menuPointer - 1];
			s[2] = menu_setting_bl[menuPointer + 1];
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
	File root, filename;
	char c;
	String s, tem;
	bool startStation = 0, trainNumFound = 0;
	byte awal, akhir;
	byte lineCount = 0;

#if SD_DEBUG
	Serial.print(F("SD card init ..."));
#endif	//#if SD_DEBUG

	digitalWrite(LCD_SS_PIN, HIGH);
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

	///updating trainDetail struct

	//find selected train files PID
	if (trainDetail.trainInfo.trainFiles.length() == 0) {
		root = SD.open("/");

		do {
			filename = root.openNextFile();
			if (!filename.isDirectory()) {
				s = filename.name();
				s.trim();
				if (s.indexOf(".PID") >= 0) {
					trainDetail.trainInfo.trainFiles = s;
					break;
				}
			}
		} while (filename);

		filename.close();
		root.close();
	}

//	if (!sdFindFile(trainDetail.trainInfo.trainFiles)) {
//#if SD_DEBUG
//		Serial.print(F("cant find \""));
//		Serial.print(trainDetail.trainInfo.trainFiles);
//		Serial.println("\"");
//#endif	//#if SD_DEBUG
//		//TODO [Mar 23, 2018, miftakur]:
//		//create default.txt by copy existing (first) *.PID files
//
//	}

	filename = SD.open(trainDetail.trainInfo.trainFiles);
	if (filename) {
		while (filename.available()) {
			c = filename.read();
			s += c;
			if (c == '\n') {
				s.trim();

				if (trainNumFound) {
					if (trainDetail.lang == ENGLISH_LANG) {
						awal = 0;
						akhir = s.indexOf(';');
						tem = s.substring(awal, akhir);
					}
					else {
						awal = s.indexOf(';') + 1;
						tem = s.substring(awal);
					}
					trainDetail.trainInfo.trainName = tem;
#if SD_DEBUG
					Serial.print(F("trainName= "));
					Serial.println(trainDetail.trainInfo.trainName);
#endif	//#if SD_DEBUG
					trainNumFound = 0;
				}

				if (s.indexOf("TRAIN ") >= 0) {
					if (trainDetail.lang == ENGLISH_LANG) {
						awal = s.indexOf(' ') + 1;
						akhir = s.indexOf(';');
						tem = s.substring(awal, akhir);
					}
					else {
						awal = s.indexOf(';') + 1;
						tem = s.substring(awal);
					}
					trainDetail.trainInfo.trainNum = tem;
#if SD_DEBUG
					Serial.print(F("trainNum= "));
					Serial.println(trainDetail.trainInfo.trainNum);
#endif	//#if SD_DEBUG
					trainNumFound = 1;
				}

				if (startStation) {
					if (s.length() > 0) {
						if (s.indexOf("AWAL")) {
							if (trainDetail.lang == ENGLISH_LANG) {
								awal = 0;
								akhir = ',';
								tem = s.substring(awal, akhir);
							}
							else {
								awal = s.indexOf(';') + 1;
								tem = s.substring(awal);
							}
							trainDetail.trainInfo.routeStart = tem;
						}
						else if (s.indexOf("AKHIR")) {
							if (trainDetail.lang == ENGLISH_LANG) {
								awal = 0;
								akhir = ',';
								tem = s.substring(awal, akhir);
							}
							else {
								awal = s.indexOf(';') + 1;
								tem = s.substring(awal);
							}
							trainDetail.trainInfo.routeEnd = tem;
						}

						if (lineCount == trainDetail.trainInfo.currentStationPosition) {
							if (trainDetail.lang == ENGLISH_LANG) {
								awal = 0;
								akhir = s.indexOf(',');
								tem = s.substring(awal, akhir);
							}
							else {
								awal = s.indexOf(';') + 1;
								tem = s.substring(awal);
							}
							trainDetail.trainInfo.currentStation = tem;
						}

						lineCount++;
					}
				}

				if (s == "//DATA//")
					startStation = 1;

				s = "";
			}
		}
		filename.close();
	}

	//find coach.txt
//	if (!sdFindFile("COACH.TXT")) {
//#if SD_DEBUG
//		Serial.println(F("cant find \"coach.txt\""));
//#endif	//#if SD_DEBUG
//	}

	filename = SD.open("COACH.TXT");
	if (filename) {
		while (filename.available()) {
			c = filename.read();
			s += c;
			if (c == '\n') {
				if (lineCount > 0 && s.length() > 0) {
					if (trainDetail.lang == ENGLISH_LANG) {
						awal = 0;
						akhir = s.indexOf(';');
						tem = s.substring(awal, akhir);
					}
					else {
						awal = s.indexOf(';') + 1;
						tem = s.substring(awal);
					}
					menu_coach_list[lineCount - 1] = tem;
				}

				lineCount++;
				s = "";
			}
		}	//filename.available()
	}
}

void sdFailedHandler()
{
}

//bool sdFindFile(String filename)
//{
//	bool ret = 0;
//	File root, listFile;
//	String s;
//
//	root = SD.open("/");
//
//	do {
//		listFile = root.openNextFile();
//		if (!listFile.isDirectory()) {
//			s = listFile.name();
//			if (s == filename) {
//				ret = 1;
//				break;
//			}
//		}
//	} while (listFile);
//
//	listFile.close();
//	root.close();
//
//	return ret;
//}

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

	if (buttonVal != _prevButtonVal || _update) {
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
	if ((button == DISP_STEP_SELECT) || (button == DISP_STEP_BACK) || (button == DISP_STEP_NEXT)
			|| (button == DISP_STEP_PREV))
		lcdPageUpdate(button);

#if BUTTON_DEBUG
	Serial.println(button, BIN);
#endif	//#if BUTTON_DEBUG

}

byte btnRead()
{
	byte ret = 0, a;

	for ( a = 0; a < 4; a++ )
		bitWrite(ret, a, !digitalRead(button[a]));

	return ret;
}
