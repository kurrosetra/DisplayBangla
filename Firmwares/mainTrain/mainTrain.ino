#include <SPI.h>
#include <SD.h>
#include "U8glib.h"
#include "trainDetail.h"
#include "storageBank.h"

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
const byte button[4] =
		{ A1/*RIGHT / SELECT*/, A3/*LEFT / BACK*/, A0 /*DOWN / NEXT*/, A2 /*UP / PREV*/};
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
	DISP_MENU_CURRENT_STATION,
	DISP_MENU_TRAIN_ROUTE,
	DISP_MENU_TRAIN_NAME,
	DISP_MENU_LANGUAGE,
	DISP_MENU_MASTER_MODE,
	DISP_MENU_GPS_MODE,
	DISP_MENU_COACH,
	DISP_MENU_SYSTEM_INFO,
	DISP_MENU_COACH_TYPE,
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

static const byte menu_setting_length = 8;
const char *menu_setting_en[menu_setting_length] = { "Current Station", "Train Route", "Train Name",
		"Language", "Master / Slave Mode", "Manual / GPS Mode", "Coach ID", "System Info" };
const char *menu_setting_bl[menu_setting_length] = { "~Current Station", "~Train Route",
		"~Train Name", "~Language", "~Master / Slave Mode", "~Manual / GPS Mode", "~Coach ID",
		"~System Info" };
static const byte menu_coach_list_length = 20;
String menu_coach_list_en[menu_coach_list_length];
String menu_coach_list_bl[menu_coach_list_length];
String number_conversion[menu_coach_list_length];

byte displayState = DISP_IDLE;
Train_Detail trainDetail;
Train_Detail _prevTrainDetail;
Train_Detail temTrain[3];
byte currentDisplayState = DISP_IDLE;
byte menuPointer = 0;
bool coachPointer = 0;
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

/**
 * *******************************************************************************
 * Storage Global variables
 * *******************************************************************************
 */
const byte blockNum = 5;
const byte blockSize = 100;
const byte dataSize = 7;
byte settingChange = 0;

StorageBank storage(blockNum, blockSize, dataSize);

uint32_t IDLE_TIMEOUT = 10000;
uint32_t idleTimer = 500;

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

	if (storage.init()) {
#if DEBUG
		String errMsg = "";
		storage.errorMessage(&errMsg);
		Serial.println(errMsg);
#endif	//#if DEBUG
	}

	sdInit();
	btnInit();
	lcdInit();

	lcdPageUpdate(DISP_STEP_NEXT);

	Serial.println(F("no error"));
}

void loop()
{
	if (idleTimer && millis() > idleTimer) {
		idleTimer = 0;

#if DEBUG
		Serial.println(F("idle timeout"));
#endif	//#if DEBUG

		if (settingChange) {
			trainDetail = _prevTrainDetail;
			settingChange = 0;
		}

		displayState = DISP_MAIN;
		lcdPageUpdate(DISP_STEP_BACK);
	}

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
//	static Train_Detail _prevTrainDetail;
	byte a;

	switch (displayState)
	{
	case DISP_IDLE:
		if (dispStep != DISP_STEP_NONE) {
			if (dispStep != DISP_STEP_BACK) {
				//change display to main page with backlight ON
				lcdBacklight(HIGH);
				lcdPageChange (lcdPageMain);
				displayState = DISP_MAIN;
			}
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
			lcdPageChange (lcdPageMain);
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
			displayState = menuPointer + 3;

			_prevTrainDetail = trainDetail;
			if (displayState == DISP_MENU_TRAIN_NAME) {
				for ( a = 0; a < 3; a++ )
					temTrain[a] = trainDetail;
			}
			settingChange = 1;

#if DEBUG
			Serial.println(displayState);
#endif	//#if DEBUG

			lcdPageChange (lcdPageSubMenu);
			break;
		}
		break;
	case DISP_MENU_TRAIN_ROUTE:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
		case DISP_STEP_NEXT:
			trainDetail.trainInfo.trainRoute = !trainDetail.trainInfo.trainRoute;
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//change back
			trainDetail = _prevTrainDetail;
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_SELECT:
			displayState = DISP_MENU;
			if (trainDetail.trainInfo.trainRoute)
				trainDetail.trainInfo.currentStationPosition =
						trainDetail.trainInfo.lastStationPosition;
			else
				trainDetail.trainInfo.currentStationPosition = 0;
			sdUpdateTrainDetail(&trainDetail);

			saveSetting();

			lcdPageChange(lcdPageMenu);
			break;
		}
		break;
	case DISP_MENU_TRAIN_NAME:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
			if (temTrain[1].trainInfo.currentTrainPosition == 0)
				temTrain[1].trainInfo.currentTrainPosition =
						temTrain[1].trainInfo.lastTrainPosition;
			else
				temTrain[1].trainInfo.currentTrainPosition--;
			//if new currentTrainPosition is not *.PID
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_NEXT:
			//
			if (temTrain[1].trainInfo.currentTrainPosition
					== temTrain[1].trainInfo.lastTrainPosition)
				temTrain[1].trainInfo.currentTrainPosition = 0;
			else
				temTrain[1].trainInfo.currentTrainPosition++;
			//if new currentTrainPosition is not *.PID
			lcdPageChange(lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//nothing changed
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_SELECT:
			//if trainDetail changed
			if ((trainDetail.trainInfo.trainID != temTrain[1].trainInfo.trainID)
					&& (temTrain[1].trainInfo.trainID != 0)) {

				trainDetail = temTrain[1];
				trainDetail.trainInfo.trainRoute = 0;
				trainDetail.trainInfo.currentStationPosition = 0;
//				sdUpdateTrainDetail(&trainDetail);
				saveSetting();
			}
			displayState = DISP_MENU;
			lcdPageChange(lcdPageMenu);
			break;
		}
		break;
	case DISP_MENU_CURRENT_STATION:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
			if (trainDetail.trainInfo.trainRoute) {
				if (trainDetail.trainInfo.currentStationPosition
						== trainDetail.trainInfo.lastStationPosition)
					trainDetail.trainInfo.currentStationPosition = 0;
				else
					trainDetail.trainInfo.currentStationPosition++;
			}
			else {
				if (trainDetail.trainInfo.currentStationPosition == 0)
					trainDetail.trainInfo.currentStationPosition =
							trainDetail.trainInfo.lastStationPosition;
				else
					trainDetail.trainInfo.currentStationPosition--;
			}
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_NEXT:
			if (trainDetail.trainInfo.trainRoute) {
				if (trainDetail.trainInfo.currentStationPosition == 0)
					trainDetail.trainInfo.currentStationPosition =
							trainDetail.trainInfo.lastStationPosition;
				else
					trainDetail.trainInfo.currentStationPosition--;
			}
			else {
				if (trainDetail.trainInfo.currentStationPosition
						== trainDetail.trainInfo.lastStationPosition)
					trainDetail.trainInfo.currentStationPosition = 0;
				else
					trainDetail.trainInfo.currentStationPosition++;
			}
			lcdPageChange(lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//change back
			trainDetail = _prevTrainDetail;
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_SELECT:
			displayState = DISP_MENU;

			saveSetting();

			lcdPageChange(lcdPageMenu);
			break;
		}
		break;
	case DISP_MENU_LANGUAGE:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
		case DISP_STEP_NEXT:
			trainDetail.lang = !trainDetail.lang;
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//change back
			trainDetail = _prevTrainDetail;
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_SELECT:
			displayState = DISP_MENU;

			saveSetting();

			lcdPageChange(lcdPageMenu);
			break;
		}
		break;
	case DISP_MENU_MASTER_MODE:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
		case DISP_STEP_NEXT:
			trainDetail.masterMode = !trainDetail.masterMode;
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//change back
			trainDetail = _prevTrainDetail;
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_SELECT:
			displayState = DISP_MENU;

			saveSetting();

			lcdPageChange(lcdPageMenu);
			break;
		}
		break;
	case DISP_MENU_GPS_MODE:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
		case DISP_STEP_NEXT:
			trainDetail.gpsMode = !trainDetail.gpsMode;
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//change back
			trainDetail = _prevTrainDetail;
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_SELECT:
			displayState = DISP_MENU;

			saveSetting();

			lcdPageChange(lcdPageMenu);
			break;
		}
		break;
	case DISP_MENU_COACH:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
		case DISP_STEP_NEXT:
			if (coachPointer)
				coachPointer = 0;
			else
				coachPointer = 1;
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case DISP_STEP_SELECT:
			if (coachPointer)
				displayState = DISP_MENU_COACH_NUMBER;
			else
				displayState = DISP_MENU_COACH_TYPE;
			_prevTrainDetail = trainDetail;
			lcdPageChange(lcdPageSubMenu);
			break;
		}
		break;
	case DISP_MENU_SYSTEM_INFO:
		break;
	case DISP_MENU_COACH_TYPE:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
			Serial.print(trainDetail.coach.nameID);
			if (trainDetail.coach.nameID == 0)
				trainDetail.coach.nameID = trainDetail.coach.typeNum;
			else
				trainDetail.coach.nameID--;
			Serial.print(trainDetail.coach.nameID);

			if (trainDetail.lang == ENGLISH_LANG)
				trainDetail.coach.name = menu_coach_list_en[trainDetail.coach.nameID];
			else
				trainDetail.coach.name = menu_coach_list_bl[trainDetail.coach.nameID];

			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_NEXT:
			Serial.print(trainDetail.coach.nameID);
			if (trainDetail.coach.nameID >= trainDetail.coach.typeNum)
				trainDetail.coach.nameID = 0;
			else
				trainDetail.coach.nameID++;
			Serial.print(trainDetail.coach.nameID);

			if (trainDetail.lang == ENGLISH_LANG)
				trainDetail.coach.name = menu_coach_list_en[trainDetail.coach.nameID];
			else
				trainDetail.coach.name = menu_coach_list_bl[trainDetail.coach.nameID];

			lcdPageChange(lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//change back
			trainDetail = _prevTrainDetail;
			displayState = DISP_MENU_COACH;
			lcdPageChange(lcdPageSubMenu);
			break;
		case DISP_STEP_SELECT:
			displayState = DISP_MENU_COACH;

			saveSetting();

			lcdPageChange(lcdPageSubMenu);
			break;
		}
		break;
	case DISP_MENU_COACH_NUMBER:
		switch (dispStep)
		{
		case DISP_STEP_PREV:
			if (trainDetail.coach.number == 0)
				trainDetail.coach.number = menu_coach_list_length - 1;
			else
				trainDetail.coach.number--;
			lcdPageChange (lcdPageSubMenu);
			break;
		case DISP_STEP_NEXT:
			if (trainDetail.coach.number == menu_coach_list_length - 1)
				trainDetail.coach.number = 0;
			else
				trainDetail.coach.number++;
			lcdPageChange(lcdPageSubMenu);
			break;
		case DISP_STEP_BACK:
			//change back
			trainDetail = _prevTrainDetail;
			displayState = DISP_MENU_COACH;
			lcdPageChange(lcdPageSubMenu);
			break;
		case DISP_STEP_SELECT:
			displayState = DISP_MENU_COACH;

			saveSetting();

			lcdPageChange(lcdPageSubMenu);
			break;
		}
		break;
	case DISP_MENU_CHANGE_REQ:
		break;
	}
}

void lcdPageChange(void (*updatePage)())
{
	digitalWrite(SD_SS_PIN, HIGH);
	(*updatePage)();
	digitalWrite(LCD_SS_PIN, HIGH);
}

void lcdPageMain()
{
	String title, s[4];
	u8g_uint_t w, d;

	//HEADER
	if (trainDetail.lang == ENGLISH_LANG)
		title = F("Bangladesh Railway");
	else
		title = F("~Bangladesh Railway~");

	//KONTEN 0
	s[0] = trainDetail.trainInfo.trainName;
	//KONTEN 1
	s[1] = trainDetail.trainInfo.currentStationName;
	//KONTEN 2
	s[2] = trainDetail.coach.name;
	s[2] += ' ';
	if (trainDetail.lang == ENGLISH_LANG)
		s[2] += trainDetail.coach.number + 1;
	else
		s[2] += number_conversion[trainDetail.coach.number];
	//KONTEN 3
	if (trainDetail.masterMode) {
		if (trainDetail.lang == ENGLISH_LANG)
			s[3] = F("Master");
		else
			s[3] = F("~Master~");
	}
	else {
		if (trainDetail.lang == ENGLISH_LANG)
			s[3] = F("Slave");
		else
			s[3] = F("~Slave~");
	}
	if (trainDetail.lang == ENGLISH_LANG)
		s[3] += ';';
	else
		s[3] += "~;~";
	if (trainDetail.gpsMode) {
		if (trainDetail.lang == ENGLISH_LANG)
			s[3] += F("GPS");
		else
			s[3] += F("~GPS~");
	}
	else {
		if (trainDetail.lang == ENGLISH_LANG)
			s[3] += F("Manual");
		else
			s[3] += F("~Manual~");
	}

	lcd.firstPage();
	do {
		//HEADER
		w = lcd.getWidth();
		lcd.setFont(u8g_font_ncenB08);
		lcd.setFontRefHeightText();
		lcd.setFontPosTop();
		d = (w - lcd.getStrWidth(title.c_str())) / 2;
		lcd.drawStr(d, 0, title.c_str());

		//KONTEN 1
		lcd.setFont(u8g_font_5x7r);
		d = (w - lcd.getStrWidth(s[0].c_str())) / 2;
		lcd.drawStr(d, 25, s[0].c_str());
		//KONTEN 2
		d = (w - lcd.getStrWidth(s[1].c_str())) / 2;
		lcd.drawStr(d, 35, s[1].c_str());
		//KONTEN 3
		d = (w - lcd.getStrWidth(s[2].c_str())) / 2;
		lcd.drawStr(d, 45, s[2].c_str());
		//KONTEN 4
		d = (w - lcd.getStrWidth(s[3].c_str())) / 2;
		lcd.drawStr(d, 63, s[3].c_str());
	} while (lcd.nextPage());
}

void lcdPageMenu()
{
	String title, s[3];

	settingChange = 0;

	//HEADER
	if (trainDetail.lang == ENGLISH_LANG)
		title = F("Select Menu:");
	else
		title = F("~Select Menu~");

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

	lcd.firstPage();
	do {
		lcd.setFont(u8g_font_ncenB08);
		lcd.setFontRefHeightText();
		lcd.setFontPosTop();
		lcd.drawStr(0, 0, title.c_str());

		lcd.setFont(u8g_font_5x7);

		lcd.drawStr(0, 30, s[0].c_str());

		lcd.setDefaultForegroundColor();
		lcd.drawBox(0, 37, lcd.getStrWidth(s[1].c_str()) + 1, 10);
		lcd.setDefaultBackgroundColor();
		lcd.drawStr(1, 45, s[1].c_str());
		lcd.setDefaultForegroundColor();

		lcd.drawStr(2, 60, s[2].c_str());
	} while (lcd.nextPage());
}

void lcdPageSubMenu()
{
	String title, s[3];
	Train_Detail td[3];
	byte highlightLine = 1;
	byte a, b[3];

	//HEADER
	if (trainDetail.lang == ENGLISH_LANG)
		title = menu_setting_en[menuPointer];
	else
		title = menu_setting_bl[menuPointer];

	//MENU CONTENT
	switch (displayState)
	{
	case DISP_MENU_TRAIN_NAME:
		digitalWrite(LCD_SS_PIN, HIGH);
		sdFindTrain(s);
		digitalWrite(SD_SS_PIN, HIGH);

		highlightLine = 1;
		break;
	case DISP_MENU_TRAIN_ROUTE:
		if (trainDetail.trainInfo.trainRoute == 0) {
			s[0] = trainDetail.trainInfo.routeStart;
			s[2] = trainDetail.trainInfo.routeEnd;
		}
		else {
			s[0] = trainDetail.trainInfo.routeEnd;
			s[2] = trainDetail.trainInfo.routeStart;
		}	//(trainDetail.trainInfo.trainRoute == 0)
		if (trainDetail.lang == ENGLISH_LANG)
			s[1] = F("To");
		else
			s[1] = F("~To~");

		highlightLine = 0;
		break;
	case DISP_MENU_CURRENT_STATION:
		digitalWrite(LCD_SS_PIN, HIGH);
		sdFindStation(s);
		digitalWrite(SD_SS_PIN, HIGH);

		highlightLine = 1;
		break;
	case DISP_MENU_LANGUAGE:
		s[1] = F("ENGLISH");
		s[2] = F("BANGLADESH");
		if (trainDetail.lang == ENGLISH_LANG)
			highlightLine = 1;
		else
			highlightLine = 2;
		break;
	case DISP_MENU_MASTER_MODE:
		if (trainDetail.lang == ENGLISH_LANG) {
			s[1] = F("MASTER");
			s[2] = F("SLAVE");
		}
		else {
			s[1] = F("~MASTER");
			s[2] = F("~SLAVE");
		}
		if (trainDetail.masterMode)
			highlightLine = 1;
		else
			highlightLine = 2;
		break;
	case DISP_MENU_GPS_MODE:
		if (trainDetail.lang == ENGLISH_LANG) {
			s[1] = F("GPS");
			s[2] = F("MANUAL");
		}
		else {
			s[1] = F("~GPS");
			s[2] = F("~MANUAL");
		}
		if (trainDetail.gpsMode)
			highlightLine = 1;
		else
			highlightLine = 2;
		break;
	case DISP_MENU_COACH:
		s[1] = trainDetail.coach.name;

		if (trainDetail.lang == ENGLISH_LANG)
			s[2] = trainDetail.coach.number + 1;
		else
			s[2] = number_conversion[trainDetail.coach.number];

		if (coachPointer)
			highlightLine = 2;
		else
			highlightLine = 1;
		break;
	case DISP_MENU_COACH_TYPE:
		b[1] = trainDetail.coach.nameID;
		if (trainDetail.coach.nameID == 0) {
			b[0] = trainDetail.coach.typeNum;
			b[2] = 1;
		}
		else if (trainDetail.coach.nameID >= trainDetail.coach.typeNum) {
			b[0] = trainDetail.coach.typeNum - 1;
			b[2] = 0;
		}
		else {
			b[0] = b[1]--;
			b[2] = b[1]++;
		}

		if (trainDetail.lang == ENGLISH_LANG) {
			for ( a = 0; a < 3; a++ )
				s[a] = menu_coach_list_en[b[a]];
		}
		else {
			for ( a = 0; a < 3; a++ )
				s[a] = menu_coach_list_bl[b[a]];
		}

		highlightLine = 1;
		break;
	case DISP_MENU_COACH_NUMBER:
		if (trainDetail.lang == ENGLISH_LANG) {
			if (trainDetail.coach.number == 0) {
				s[1] = 1;
				s[0] = menu_coach_list_length;
				s[2] = 2;
			}
			else if (trainDetail.coach.number >= menu_coach_list_length - 1) {
				s[1] = menu_coach_list_length;
				s[0] = menu_coach_list_length - 1;
				s[2] = 1;
			}
			else {
				s[1] = trainDetail.coach.number + 1;
				s[0] = trainDetail.coach.number;
				s[2] = trainDetail.coach.number + 2;
			}
		}
		else {
			if (trainDetail.coach.number == 0) {
				s[1] = number_conversion[0];
				s[0] = number_conversion[menu_coach_list_length - 1];
				s[2] = number_conversion[1];
			}
			else if (trainDetail.coach.number >= menu_coach_list_length - 1) {
				s[1] = number_conversion[menu_coach_list_length - 1];
				s[0] = number_conversion[menu_coach_list_length - 2];
				s[2] = number_conversion[0];
			}
			else {
				s[1] = number_conversion[trainDetail.coach.number];
				s[0] = number_conversion[trainDetail.coach.number - 1];
				s[2] = number_conversion[trainDetail.coach.number + 1];
			}
		}
		highlightLine = 1;
		break;
	}

	lcd.firstPage();
	do {
		//HEADER
		lcd.setFont(u8g_font_ncenB08);
		lcd.setFontRefHeightText();
		lcd.setFontPosTop();
		lcd.drawStr(0, 0, title.c_str());

		//PRINT MENU CONTENT
		lcd.setFont(u8g_font_5x7);
		for ( a = 0; a < 3; a++ ) {
			if (highlightLine == a) {
				lcd.setDefaultForegroundColor();
				lcd.drawBox(0, 22 + (15 * a), lcd.getStrWidth(s[a].c_str()) + 1, 10);
				lcd.setDefaultBackgroundColor();
				lcd.drawStr(0, 30 + (15 * a), s[a].c_str());
				lcd.setDefaultForegroundColor();
			}
			else {
				lcd.drawStr(0, 30 + (15 * a), s[a].c_str());
			}
		}
	} while (lcd.nextPage());
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

	loadSetting();

	//update coach list
	sdCoachUpdate();
	if (trainDetail.coach.name.length() == 0) {
		trainDetail.coach.nameID = 0;
		if (trainDetail.lang == ENGLISH_LANG)
			trainDetail.coach.name = menu_coach_list_en[trainDetail.coach.nameID];
		else
			trainDetail.coach.name = menu_coach_list_bl[trainDetail.coach.nameID];
		trainDetail.coach.number = 0;
	}

	///updating trainDetail struct
	if (!sdUpdateTrainDetail(&trainDetail)) {
#if DEBUG
		Serial.println(F("train not found!"));
#endif	//#if DEBUG
	}

}

void sdFailedHandler()
{
}

void sdCoachUpdate()
{
	char c;
	String s, tem;
	byte awal, akhir, index;
	bool startCoachNumber = 0;
	uint16_t lineCount = 0;
	File filename = SD.open("COACH.TXT");
	byte a;

	if (filename) {
		while (filename.available()) {
			c = filename.read();
			s += c;
			if (c == '\n') {
				s.trim();
				if (!startCoachNumber) {
					if (s.indexOf(F("//DATA//")) >= 0) {
						startCoachNumber = 1;
						trainDetail.coach.typeNum = lineCount - 2;
					}
					else {
						if (lineCount > 0 && s.length() > 0) {
							awal = 0;
							akhir = s.indexOf(';');
							tem = s.substring(awal, akhir);
							menu_coach_list_en[lineCount - 1] = tem;

							tem = s.substring(akhir + 1);
							menu_coach_list_bl[lineCount - 1] = tem;
						}
					}
				}
				else {
					if (s.length() > 0) {
						awal = s.indexOf(';');
						tem = s.substring(0, awal);
						index = tem.toInt() - 1;
						tem = s.substring(awal + 1);
						number_conversion[index] = tem;
					}
				}

				lineCount++;
				s = "";
			}
		}	//filename.available()
	}
	filename.close();

	//update coach info
	if (trainDetail.lang == ENGLISH_LANG)
		trainDetail.coach.name = menu_coach_list_en[trainDetail.coach.nameID];
	else
		trainDetail.coach.name = menu_coach_list_bl[trainDetail.coach.nameID];

	trainDetail.coach.typeNum = 0;
	for ( a = 0; a < menu_coach_list_length; a++ ) {
		if (menu_coach_list_en[a].length() > 0)
			trainDetail.coach.typeNum++;
		else {
			trainDetail.coach.typeNum--;
			break;
		}
	}
}

bool sdUpdateTrainDetail(Train_Detail * td)
{
	bool ret = 0;
	File root, filename;
	String s, tem;
	char c;
	bool trainNumFound = 0, startStation = 0;
	byte awal, akhir;
	uint16_t lineCount = 0;

	//find selected train files PID
	if (td->trainInfo.trainFiles.length() == 0) {
		root = SD.open("/");

		do {
			filename = root.openNextFile();
			//no more files
			if (!filename)
				break;
			if (!filename.isDirectory()) {
				s = filename.name();
				s.trim();

				if (s.indexOf(".PID") >= 0) {
					if (td->trainInfo.trainID == 0) {
						td->trainInfo.trainFiles = s;

						td->trainInfo.currentTrainPosition = 0;
						ret = 1;
					}
					else {
						//ex. 	KA41KA~1.PID
						//		KA123T~1.PID
						c = s.charAt(4);
						if (c >= '0' || c <= '9')
							akhir = 4;
						else
							akhir = 3;
						tem = s.substring(2, akhir);
						if (td->trainInfo.trainID == tem.toInt()) {
							td->trainInfo.trainFiles = s;
							td->trainInfo.currentTrainPosition = lineCount;
							ret = 1;
						}
					}	// td->trainInfo.trainID == 0
					lineCount++;
				}
			}
		} while (filename);

		filename.close();
		root.close();

		//file not found
		if (!ret)
			return 0;

		td->trainInfo.lastTrainPosition = lineCount - 1;
	}	//(td->trainInfo.trainFiles.length() == 0)

	lineCount = 0;
	filename = SD.open(td->trainInfo.trainFiles);
	if (filename) {
		while (filename.available()) {
			c = filename.read();
			s += c;
			if (c == '\n') {
				s.trim();

				if (trainNumFound) {
					if (td->lang == ENGLISH_LANG) {
						akhir = s.indexOf(';');
						tem = s.substring(0, akhir);
					}
					else {
						awal = s.indexOf(';') + 1;
						tem = s.substring(awal);
					}
					td->trainInfo.trainName = tem;
					trainNumFound = 0;
				}
				else {
					if (s.indexOf("KA ") >= 0 && !startStation) {
						awal = s.indexOf(' ') + 1;
						akhir = s.indexOf(';');
						tem = s.substring(awal, akhir);
						td->trainInfo.trainID = tem.toInt();
						if (td->lang != ENGLISH_LANG) {
							awal = s.indexOf(';') + 1;
							tem = s.substring(awal);
						}
						td->trainInfo.trainNum = tem;
						trainNumFound = 1;
					}
				}

				if (startStation) {
					if (s.length() > 0) {
						if (s.indexOf("AWAL") >= 0) {
							if (td->lang == ENGLISH_LANG) {
								akhir = s.indexOf(',');
								tem = s.substring(0, akhir);
							}
							else {
								awal = s.indexOf(';') + 1;
								tem = s.substring(awal);
							}
							td->trainInfo.routeStart = tem;
						}
						else if (s.indexOf("AKHIR") >= 0) {
							if (td->lang == ENGLISH_LANG) {
								akhir = s.indexOf(',');
								tem = s.substring(0, akhir);
							}
							else {
								awal = s.indexOf(';') + 1;
								tem = s.substring(awal);
							}
							td->trainInfo.routeEnd = tem;
							td->trainInfo.lastStationPosition = lineCount;
						}

						if (lineCount == td->trainInfo.currentStationPosition) {
							if (td->lang == ENGLISH_LANG) {
								akhir = s.indexOf(',');
								tem = s.substring(0, akhir);
							}
							else {
								awal = s.indexOf(';') + 1;
								tem = s.substring(awal);
							}
							td->trainInfo.currentStationName = tem;
						}

						lineCount++;
					}	//s.length() > 0
				}  //startStation
				else {
					if (s.indexOf("//DATA//") >= 0) {
						startStation = 1;
					}
				}

				s = "";
			}
		}
		filename.close();
	}

	//update coach info
	if (td->lang == ENGLISH_LANG)
		td->coach.name = menu_coach_list_en[td->coach.nameID];
	else
		td->coach.name = menu_coach_list_bl[td->coach.nameID];

#if DEBUG
	Serial.print(F("trainID= "));
	Serial.println(td->trainInfo.trainID);
	Serial.print(F("trainPos= "));
	Serial.print(td->trainInfo.currentTrainPosition);
	Serial.print(F(" of "));
	Serial.println(td->trainInfo.lastTrainPosition);
	Serial.print(F("Filename: "));
	Serial.println(td->trainInfo.trainFiles);

	Serial.print(F("Train: "));
	Serial.print(td->trainInfo.trainNum);
	Serial.print(' ');
	Serial.println(td->trainInfo.trainName);

	Serial.print(F("route: "));
	Serial.print(td->trainInfo.routeStart);
	Serial.print(F(" to "));
	Serial.println(td->trainInfo.routeEnd);

	Serial.print(F("Station pos= "));
	Serial.print(td->trainInfo.currentStationPosition);
	Serial.print(F(" of "));
	Serial.println(td->trainInfo.lastStationPosition);
	Serial.print(F("Station: "));
	Serial.println(td->trainInfo.currentStationName);

	Serial.print(F("Coach: "));
	Serial.print(td->coach.name);
	Serial.print(F(" - "));
	Serial.println(td->coach.number);
	Serial.print(F("num of coach's type= "));
	Serial.println(td->coach.typeNum + 1);
#endif	//#if DEBUG

	return ret;
}

void sdFindStation(String *str)
{
	File filename;
	char c;
	byte awal, akhir;
	String line, tem;
	uint16_t lineCount = 0;
	bool startStation = 0;
	uint16_t posLine[3];

	if (trainDetail.trainInfo.currentStationPosition == 0) {
		posLine[1] = 0;
		if (trainDetail.trainInfo.trainRoute) {
			posLine[0] = 1;
			posLine[2] = trainDetail.trainInfo.lastStationPosition;
		}
		else {
			posLine[0] = trainDetail.trainInfo.lastStationPosition;
			posLine[2] = 1;
		}
	}
	else if (trainDetail.trainInfo.currentStationPosition
			== trainDetail.trainInfo.lastStationPosition) {
		posLine[1] = trainDetail.trainInfo.lastStationPosition;
		if (trainDetail.trainInfo.trainRoute) {
			posLine[0] = 0;
			posLine[2] = trainDetail.trainInfo.lastStationPosition - 1;
		}
		else {
			posLine[0] = trainDetail.trainInfo.lastStationPosition - 1;
			posLine[2] = 0;
		}
	}
	else {
		posLine[1] = trainDetail.trainInfo.currentStationPosition;
		if (trainDetail.trainInfo.trainRoute) {
			posLine[0] = trainDetail.trainInfo.currentStationPosition + 1;
			posLine[2] = trainDetail.trainInfo.currentStationPosition - 1;
		}
		else {
			posLine[0] = trainDetail.trainInfo.currentStationPosition - 1;
			posLine[2] = trainDetail.trainInfo.currentStationPosition + 1;
		}
	}

	filename = SD.open(trainDetail.trainInfo.trainFiles);
	if (filename) {
		while (filename.available()) {
			c = filename.read();
			line += c;
			if (c == '\n') {
				line.trim();

				if (startStation) {
					// found target lineCount
					if ((lineCount == posLine[0]) || (lineCount == posLine[1])
							|| (lineCount == posLine[2])) {
						tem = "";
						akhir = line.indexOf(',');
						tem = line.substring(0, akhir);
						if (trainDetail.lang != ENGLISH_LANG) {
							awal = line.indexOf(';') + 1;
							tem = line.substring(awal);
						}
					}  //((lineCount == posLine[0]) || (lineCount == posLine[1]) || (lineCount == posLine[2]))

					if (lineCount == posLine[0])
						str[0] = tem;
					else if (lineCount == posLine[1])
						str[1] = tem;
					else if (lineCount == posLine[2])
						str[2] = tem;

					lineCount++;
				}	//(startStation)
				else {
					if (line.indexOf("//DATA//") >= 0) {
						startStation = 1;
						lineCount = 0;
					}	//(line.indexOf("//DATA//") >= 0)
				}

				line = "";
			}	//(c == '\n')
		}	//filename.available()
		filename.close();
	}	//(filename)
}

void sdFindTrain(String *str)
{
	File root, fileList;
	char c;
	String s, tem;
	byte awal, akhir, a;
	uint16_t lineCount = 0;
	bool startTrain = 0;
	bool finishParsing = 0;
	uint16_t filePos[3];
	String trainNum[3];
	String trainName[3];

	filePos[1] = temTrain[1].trainInfo.currentTrainPosition;
	if (filePos[1] == 0) {
		filePos[0] = temTrain[1].trainInfo.lastTrainPosition;
		filePos[2] = 1;
	}
	else if (filePos[1] == temTrain[1].trainInfo.lastTrainPosition) {
		filePos[0] = temTrain[1].trainInfo.lastTrainPosition - 1;
		filePos[2] = 0;
	}
	else {
		filePos[0] = filePos[1] - 1;
		filePos[2] = filePos[1] + 1;
	}

	//find file's name for each temTrain
	root = SD.open("/");

	fileList = root.openNextFile();
	while (fileList) {

		if (!fileList.isDirectory()) {
			tem = fileList.name();
			if (tem.indexOf(F(".PID")) > 0) {
				for ( a = 0; a < 3; a++ )
					if (lineCount == filePos[a])
						temTrain[a].trainInfo.trainFiles = tem;
				lineCount++;
			}
		}	//(!filename.isDirectory())

		fileList = root.openNextFile();
	}

	fileList.close();
	root.close();

	//update temTrain's number & name
	for ( a = 0; a < 3; a++ ) {
		finishParsing = 0;
		startTrain = 0;
		//open file
		fileList = SD.open(temTrain[a].trainInfo.trainFiles);

		if (fileList) {
			while (fileList.available() && !finishParsing) {
				c = fileList.read();
				s += c;

				if (c == '\n') {
					if (!startTrain) {
						if (s.indexOf(F("KA ")) >= 0) {
							awal = 3;
							akhir = s.indexOf(';');

							trainNum[a] = s.substring(awal, akhir);
							temTrain[a].trainInfo.trainID = trainNum[a].toInt();

							if (temTrain[a].lang != ENGLISH_LANG)
								trainNum[a] = s.substring(akhir + 1);

							startTrain = 1;
						}
					}
					else {
						akhir = s.indexOf(';');

						if (temTrain[a].lang == ENGLISH_LANG)
							trainName[a] = s.substring(0, akhir);
						else
							trainName[a] = s.substring(akhir + 1);

						finishParsing = 1;
					}
					s = "";
				}
			}	//fileList.available()
			fileList.close();
		}	//(fileList)
	}

	for ( a = 0; a < 3; a++ )
		str[a] = trainNum[a] + String(' ') + trainName[a];

#if DEBUG
	for ( a = 0; a < 3; a++ ) {
		Serial.print(F("str["));
		Serial.print(a);
		Serial.print(F("]= "));
		Serial.println(str[a]);
	}
#endif	//#if DEBUG

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

	idleTimer = millis() + IDLE_TIMEOUT;
}

byte btnRead()
{
	byte ret = 0, a;

	for ( a = 0; a < 4; a++ )
		bitWrite(ret, a, !digitalRead(button[a]));

	return ret;
}

/**
 * *******************************************************************************
 * Storage functions
 * *******************************************************************************
 */
void loadSetting()
{
	byte data[dataSize];

	//create non-null data
	trainDetail.trainInfo.trainID = 40;
	trainDetail.trainInfo.currentStationPosition = 0;
	trainDetail.lang = ENGLISH_LANG;
	trainDetail.masterMode = MASTER_MODE;
	trainDetail.gpsMode = MANUAL_MODE;
	trainDetail.coach.nameID = 0;
	trainDetail.coach.number = 0;

	storage.read(data);

	if (data[0] != 0xFF && data[1] != 0xFF)
		trainDetail.trainInfo.trainID = (uint16_t) data[1] << 8 | data[0];

	if (data[2] != 0xFF && data[3] != 0xFF)
		trainDetail.trainInfo.currentStationPosition = (uint16_t) data[3] << 8 | data[2];

	if (data[4] != 0xFF)
		trainDetail.coach.nameID = data[4];

	if (data[5] != 0xFF)
		trainDetail.coach.number = data[5];

	if (data[6] != 0xFF) {
		trainDetail.lang = bitRead(data[6], 0);
		trainDetail.masterMode = bitRead(data[6], 1);
		trainDetail.gpsMode = bitRead(data[6], 2);
	}

#if DEBUG
	Serial.print(F("trainID= "));
	Serial.println(trainDetail.trainInfo.trainID);
	Serial.print(F("currentStationPosition= "));
	Serial.println(trainDetail.trainInfo.currentStationPosition);
	Serial.print(F("coach.nameID= "));
	Serial.println(trainDetail.coach.nameID);
	Serial.print(F("coach.number= "));
	Serial.println(trainDetail.coach.number);
	if (trainDetail.lang == ENGLISH_LANG)
		Serial.println(F("English Language"));
	else
		Serial.println(F("Bangladesh Language"));
	if (trainDetail.masterMode == MASTER_MODE)
		Serial.println(F("Master Mode"));
	else
		Serial.println(F("Slave Mode"));
	if (trainDetail.gpsMode == GPS_MODE)
		Serial.println(F("GPS Mode"));
	else
		Serial.println(F("Manual Mode"));
#endif	//#if DEBUG

}

void saveSetting()
{
	byte data[dataSize];

	//2B trainID
	data[0] = trainDetail.trainInfo.trainID & 0xFF;
	data[1] = trainDetail.trainInfo.trainID >> 8;
	//2B currentStationPos
	data[2] = trainDetail.trainInfo.currentStationPosition & 0xFF;
	data[3] = trainDetail.trainInfo.currentStationPosition >> 8;
	//1B coach's name ID
	data[4] = trainDetail.coach.nameID;
	//1B coach's number
	data[5] = trainDetail.coach.number;
	//1b language
	bitWrite(data[6], 0, trainDetail.lang);
	//1b master
	bitWrite(data[6], 1, trainDetail.masterMode);
	//1b gps
	bitWrite(data[6], 2, trainDetail.gpsMode);

	storage.write(data);

#if DEBUG
	Serial.print(F("trainID= "));
	Serial.println(trainDetail.trainInfo.trainID);
	Serial.print(F("currentStationPosition= "));
	Serial.println(trainDetail.trainInfo.currentStationPosition);
	Serial.print(F("coach.nameID= "));
	Serial.println(trainDetail.coach.nameID);
	Serial.print(F("coach.number= "));
	Serial.println(trainDetail.coach.number);
	if (trainDetail.lang == ENGLISH_LANG)
		Serial.println(F("English Language"));
	else
		Serial.println(F("Bangladesh Language"));
	if (trainDetail.masterMode == MASTER_MODE)
		Serial.println(F("Master Mode"));
	else
		Serial.println(F("Slave Mode"));
	if (trainDetail.gpsMode == GPS_MODE)
		Serial.println(F("GPS Mode"));
	else
		Serial.println(F("Manual Mode"));
#endif	//#if DEBUG

	///updating trainDetail struct
	sdUpdateTrainDetail(&trainDetail);

}
