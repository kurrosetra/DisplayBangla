#include <avr/wdt.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>
#include "U8glib.h"
#include "train.h"
#include "TinyGPS++.h"

/**
 * DECLARATION
 */

#define BOARD_KONTROLLER3A		0
#define BOARD_KONTROLLER3		1

#if BOARD_KONTROLLER3A
const String hardwareVersion = "v3a";
#elif BOARD_KONTROLLER3
const String hardwareVersion = "v3";
#endif	//BOARD_KONTROLLER3
const String softwareVersion = "1.3.8a";

#define DEBUG            	1
#if DEBUG
# define LCD_DEBUG          1
# define SD_DEBUG         	1
# define BUTTON_DEBUG       1
# define DATA_DEBUG         1
# define GPS_DEBUG          1

#endif  //#if DEBUG

#define WDT_TIMEOUT           WDTO_8S //timeout of WDT
#define WDT_RESET_TIME        500     //time to reset wdt

/**
 * PINOUT
 */

const byte MASTER_PIN = A7;
const byte LCD_BACKLIGHT_PIN = 6;
const byte LCD_SS_PIN = 4;
const byte LCD_RST_PIN = A2;
const byte SD_SS_PIN = 7;
const byte button[4] = { A3/*RIGHT / SELECT*/, A6/*LEFT / BACK*/,
		A4 /*DOWN / NEXT*/, A5 /*UP / PREV*/};
#if BOARD_KONTROLLER3A
const byte BUS_MASTER_LED_PIN = 28;
const byte DISP_RT_PIN = 29;
#elif BOARD_KONTROLLER3
#define BUS_MASTER_LED_INIT()	bitSet(DDRJ,DDJ3)
#define BUS_MASTER_LED_ON()		bitSet(PORTJ,PORTJ3)
#define BUS_MASTER_LED_OFF()	bitClear(PORTJ,PORTJ3)

#define DISP_RT_INIT()			bitSet(DDRJ,DDJ2)
#define DISP_RT_TRANSMIT()		bitSet(PORTJ,PORTJ2)
#define DISP_RT_RECEIVE()		bitClear(PORTJ,PORTJ2)
#endif	//BOARD_KONTROLLER3A

const byte BUS_RX_PIN = 40;
const byte BUS_TX_PIN = 41;
#define BUS_UART        Serial1
#define DISP_UART       Serial3

const uint32_t DEBOUNCE_DELAY = 100;

/**
 * *******************************************************************************
 * LCD Global variables
 * *******************************************************************************
 */

#define LCD_ALWAYS_ON   1

typedef enum {
	DISP_MAIN, DISP_OPS, DISP_MENU = 100, DISP_MENU_COACH = 101, // Menu of coach
	/* MASTER ONLY */
	DISP_MENU_TRAIN = 102,              // Menu to change train
	DISP_MENU_GPS = 103,              // Menu to change gps mode
} Display_State_e;

static const byte menu_setting_length = 3;
const char *menu_setting_en[menu_setting_length] = { "Coach ID", "Train Name",
		"Manual / GPS Mode" };
const char *menu_setting_bl[menu_setting_length] = { "~Coach ID", "~Train Name",
		"~Manual / GPS Mode" };
byte menuPointer = 0;
void (*pUpdatePage)(Train_Detail_t *td);
byte coachPointer = 0;

const uint16_t DATA_MAX_BUFSIZE = 256;
String dataBus = "";
String dataDisp = "";
String dataMasterCoachId = "";
bool masterConnected = false;
const uint32_t CONN_LOST_TIMEOUT = 10000;
const uint32_t BUS_SEND_TIMEOUT = 5000;
const uint32_t DISP_SEND_TIMEOUT = 5000;
uint32_t busSendTimer = BUS_SEND_TIMEOUT;
uint32_t dispSendTimer = DISP_SEND_TIMEOUT;

byte displayState = DISP_MAIN;
U8GLIB_ST7920_128X64_4X lcd(LCD_SS_PIN);

/**
 * *******************************************************************************
 * BUTTON Global Variables
 * *******************************************************************************
 */
typedef enum {
	BTN_SELECT = 0b1, /* RIGHT BUTTON */
	BTN_BACK = 0b10, /* LEFT BUTTON */
	BTN_NEXT = 0b100, /* DOWN BUTTON */
	BTN_PREV = 0b1000 /* UP BUTTON */
} Button_e;

/**
 * *******************************************************************************
 * SD Card Global variables
 * *******************************************************************************
 */
Train_Filename_t allTrainsName;
Train_Filename_t allTrainsNameBl;
bool microSDavailable = 0;
const uint32_t IDLE_TIMEOUT = 30000;
uint32_t idleTimer = 500;

/**
 * *******************************************************************************
 * GPS Global variables
 * *******************************************************************************
 */
TinyGPSPlus gps;
const uint32_t GPS_BAUD_RATE = 9600;
#if GPS_DEBUG
const uint32_t GPS_STATION_RADIUS = 200;
const uint32_t GPS_STATION_DISTANCE_LEAVE = 400;
const uint32_t GPS_STATION_DISTANCE_ENTER = 500;
#else
const uint32_t GPS_STATION_RADIUS = 500;
const uint32_t GPS_STATION_DISTANCE_LEAVE = 750;
const uint32_t GPS_STATION_DISTANCE_ENTER = 1000;
#endif	//GPS_DEBUG
bool gpsTrainInStation = 1;

/**
 * *******************************************************************************
 * STATE Global variables
 * *******************************************************************************
 */
Train_Detail_t trainParameter;
Train_Detail_t trainDisplay;

void setup() {
	wdt_enable(WDT_TIMEOUT);

	Serial.begin(115200);

#if DEBUG
	Serial.println(F("Display Bangla - INKA firmware"));
	uint32_t sdInitTimer = 0;
#else
	Serial.println(F("Display Bangla Controller Firmware"));
	Serial.print(F("s/w version: "));
	Serial.println(softwareVersion);
	Serial.print(F("h/w version: "));
	Serial.println(hardwareVersion);
#endif  //#if DEBUG

	pinMode(LCD_SS_PIN, OUTPUT);
	digitalWrite(LCD_SS_PIN, HIGH);
	pinMode(SD_SS_PIN, OUTPUT);
	digitalWrite(SD_SS_PIN, HIGH);
	pinMode(53, OUTPUT);
	digitalWrite(53, HIGH);

#if DEBUG
	sdInitTimer = micros();
#endif  //#if DEBUG

	sdInit();

#if DEBUG
	sdInitTimer = micros() - sdInitTimer;
	Serial.print(F("sdInit() time= "));
	Serial.print(sdInitTimer);
	Serial.println(F("us"));
#endif  //#if DEBUG

	coachGetParameter(&trainParameter);
	lcdInit();
	btnInit();
	dataInit();
	gpsInit();

#if DEBUG
	Serial.println(F("no error"));
#else
	delayWdt(1000);
	lcdPageChange (lcdPageMain);
#endif  //#if DEBUG

}

void loop() {
	static uint32_t refreshLcdTimer = 0;
	wdt_reset();

	btnHandler();
	gpsHandler();
	dataHandler();

	if (idleTimer) {
		if (millis() > idleTimer) {
			idleTimer = 0;
			lcdPageChange (lcdPageMain);
		}
	}  // idleTimer
	else {
		if (millis() > refreshLcdTimer) {
			refreshLcdTimer = millis() + CONN_LOST_TIMEOUT;
			lcdPageChange (lcdPageMain);
		}
	} // else idleTimer

}

/**
 * *******************************************************************************
 * LCD functions
 * *******************************************************************************
 */
void lcdInit() {
	pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
	displayState = DISP_MAIN;

#if DEBUG
	lcdPageChange (lcdPageMain);
#else
	String dispInit = "";

	lcd.firstPage();
	do {

		lcd.setFont(u8g_font_ncenB08);
		lcd.setFontRefHeightText();
		lcd.setFontPosTop();
		lcd.drawStr(0, 0, "version");

		lcd.setFont(u8g_font_5x7r);

		//KONTEN 1
		dispInit = "hw: " + hardwareVersion;
		lcd.drawStr(0, 25, dispInit.c_str());
		//KONTEN 2
		dispInit = "sw: " + softwareVersion;
		lcd.drawStr(0, 35, dispInit.c_str());
	}while (lcd.nextPage());
#endif	//DEBUG
	lcdBacklight(1);
}

void lcdBacklight(bool onf) {
#if LCD_ALWAYS_ON
	digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
#else
	digitalWrite(LCD_BACKLIGHT_PIN, onf);
#endif  //#if LCD_ALWAYS_ON

}

void lcdPageUpdate(byte dispStep) {
	char c, newChar;

	switch (displayState) {
	case DISP_MAIN:
		switch (dispStep) {
		case BTN_BACK:
			// goto menu page
			menuPointer = 0;
			lcdPageChange (lcdPageMenu);
			displayState = DISP_MENU;
			break;
		case BTN_NEXT:
			// goto ops page, with next step
			if (trainParameter.masterMode == MASTER_MODE) {
				trainDisplay = trainParameter;
				masterChangeStationState(&trainDisplay, STATION_NEXT);
				lcdPageChange (lcdPageOps);
				displayState = DISP_OPS;
			}
			break;
		case BTN_PREV:
			// goto ops page, with prev step
			if (trainParameter.masterMode == MASTER_MODE) {
				trainDisplay = trainParameter;
				masterChangeStationState(&trainDisplay, STATION_PREV);
				lcdPageChange (lcdPageOps);
				displayState = DISP_OPS;
			}
			break;
		}
		break;
	case DISP_OPS:
		switch (dispStep) {
		case BTN_BACK:
			// goto main page
			lcdPageChange (lcdPageMain);
			displayState = DISP_MAIN;
			break;
		case BTN_NEXT:
			// goto ops page, with next step
			if (trainParameter.masterMode == MASTER_MODE) {
				masterChangeStationState(&trainDisplay, STATION_NEXT);
				lcdPageChange (lcdPageOps);
				displayState = DISP_OPS;
			}
			break;
		case BTN_PREV:
			// goto ops page, with prev step
			if (trainParameter.masterMode == MASTER_MODE) {
				masterChangeStationState(&trainDisplay, STATION_PREV);
				lcdPageChange (lcdPageOps);
				displayState = DISP_OPS;
			}
			break;
		case BTN_SELECT:
			trainParameter = trainDisplay;
			if (trainParameter.masterMode == MASTER_MODE)
				masterUpdateTrainBangla();
			lcdPageChange(lcdPageMain);
			displayState = DISP_MAIN;
			break;
		}
		break;
	case DISP_MENU:
		if (trainParameter.masterMode == MASTER_MODE) {
			switch (dispStep) {
			case BTN_NEXT:
				menuPointer++;
				if (menuPointer >= menu_setting_length)
					menuPointer = 0;
				lcdPageChange (lcdPageMenu);
				break;
			case BTN_PREV:
				if (menuPointer == 0)
					menuPointer = menu_setting_length - 1;
				else
					menuPointer--;
				lcdPageChange(lcdPageMenu);
				break;
			case BTN_BACK:
				//back to DISP_MAIN state
				menuPointer = 0;
				lcdPageChange (lcdPageMain);
				displayState = DISP_MAIN;
				break;
			case BTN_SELECT:
				displayState = menuPointer + DISP_MENU + 1;
				trainDisplay = trainParameter;
				coachPointer = 0;

				lcdPageChangeSubmenu(&trainDisplay);

				break;
			}
		}  // trainParameter.masterMode == MASTER_MODE
		else {
			switch (dispStep) {
			case BTN_NEXT:
				break;
			case BTN_PREV:
				break;
			case BTN_BACK:
				//back to DISP_MAIN state
				menuPointer = 0;
				lcdPageChange (lcdPageMain);
				displayState = DISP_MAIN;
				break;
			case BTN_SELECT:
				displayState = DISP_MENU_COACH;
				trainDisplay = trainParameter;
				coachPointer = 0;

				lcdPageChangeSubmenu(&trainDisplay);
				break;
			}
		}  // else trainParameter.masterMode == MASTER_MODE
		break;
	case DISP_MENU_TRAIN:
		switch (dispStep) {
		case BTN_PREV:
			if (allTrainsName.size > 0) {
				if (trainDisplay.trainInfo.trainPosition > 0)
					trainDisplay.trainInfo.trainPosition--;
				else
					trainDisplay.trainInfo.trainPosition = allTrainsName.size
							- 1;
				masterUpdateTrain(&trainDisplay,
						allTrainsName.trainID[trainDisplay.trainInfo.trainPosition]);

				lcdPageChangeSubmenu(&trainDisplay);
			}
			break;
		case BTN_NEXT:
			if (allTrainsName.size > 0) {
				if (trainDisplay.trainInfo.trainPosition
						< allTrainsName.size - 1)
					trainDisplay.trainInfo.trainPosition++;
				else
					trainDisplay.trainInfo.trainPosition = 0;
				masterUpdateTrain(&trainDisplay,
						allTrainsName.trainID[trainDisplay.trainInfo.trainPosition]);

				lcdPageChangeSubmenu(&trainDisplay);
			}
			break;
		case BTN_BACK:
			//nothing changed
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case BTN_SELECT:
			if (allTrainsName.size > 0) {
				//if trainDetail changed
				if ((trainParameter.trainInfo.trainID
						!= trainDisplay.trainInfo.trainID)
						&& (trainDisplay.trainInfo.trainID != 0)) {

					trainParameter = trainDisplay;
					trainParameter.stationInfo.pos = 0;
					masterUpdateTrain(&trainParameter,
							allTrainsName.trainID[trainParameter.trainInfo.trainPosition]);
					masterUpdateStation(&trainParameter, 0, STATION_ARRIVED);
					if (trainParameter.masterMode == MASTER_MODE)
						masterUpdateTrainBangla();
				}
			}

			lcdPageChange (lcdPageMain);
			break;
		}
		break;
	case DISP_MENU_GPS:
		switch (dispStep) {
		case BTN_PREV:
			trainDisplay.gpsMode = !trainDisplay.gpsMode;
			lcdPageChangeSubmenu(&trainDisplay);
			break;
		case BTN_NEXT:
			trainDisplay.gpsMode = !trainDisplay.gpsMode;
			lcdPageChangeSubmenu(&trainDisplay);
			break;
		case BTN_BACK:
			//change back
			displayState = DISP_MENU;
			lcdPageChange (lcdPageMenu);
			break;
		case BTN_SELECT:
			trainParameter = trainDisplay;
			lcdPageChange (lcdPageMain);
			break;
		}
		break;
	case DISP_MENU_COACH:
		switch (dispStep) {
		case BTN_NEXT:
			// ' ' -> 'A' - 'Z' -> '0' - '9'
			c = trainDisplay.coachName.charAt(coachPointer);

			if (c == 'Z')
				newChar = '0';
			else if (c == '9')
				newChar = ' ';
			else if (((c >= 'A') && (c < 'Z')) || ((c >= '0') && (c < '9')))
				newChar = c + 1;
			else
				newChar = 'A';

			trainDisplay.coachName.setCharAt(coachPointer, newChar);
#if DEBUG
			Serial.print(newChar);
			Serial.print(F(" -> coachName= "));
			Serial.println(trainDisplay.coachName);
#endif  //#if DEBUG
			//update display
			lcdPageChangeSubmenu(&trainDisplay);
			break;
		case BTN_PREV:
			// ' ' <- 'A' - 'Z' <- '0' - '9'
			c = trainDisplay.coachName.charAt(coachPointer);

			if (c == 'A')
				newChar = ' ';
			else if (c == '0')
				newChar = 'Z';
			else if (((c > 'A') && (c <= 'Z')) || ((c > '0') && (c <= '9')))
				newChar = c - 1;
			else
				newChar = '9';

			trainDisplay.coachName.setCharAt(coachPointer, newChar);
#if DEBUG
			Serial.print(newChar);
			Serial.print(F(" -> coachName= "));
			Serial.println(trainDisplay.coachName);
#endif  //#if DEBUG
			//update display
			lcdPageChangeSubmenu(&trainDisplay);
			break;
		case BTN_BACK:
			if (coachPointer > 0) {
				coachPointer--;
				lcdPageChangeSubmenu(&trainDisplay);
			} else {
				// cancel
				displayState = DISP_MENU;
				lcd.disableCursor();
				coachPointer = 0;
				lcdPageChange (lcdPageMenu);
			}
			break;
		case BTN_SELECT:
			coachPointer++;
			if (coachPointer >= COACH_NAME_MAX_SIZE) {
				//done
				coachSetParameter(&trainParameter, trainDisplay.coachName);
				lcd.disableCursor();
				lcdPageChange (lcdPageMain);
			}
			// if last digit is space, done
			else if ((trainDisplay.coachName.charAt(coachPointer - 1) == ' ')
					|| (trainDisplay.coachName.charAt(coachPointer - 1) == 0)) {
				//done
				for (byte i = coachPointer - 1; i < COACH_NAME_MAX_SIZE; i++)
					trainDisplay.coachName.setCharAt(i, ' ');
				coachPointer = COACH_NAME_MAX_SIZE;
				trainDisplay.coachName.trim();

				coachSetParameter(&trainParameter, trainDisplay.coachName);
				lcd.disableCursor();
				lcdPageChange (lcdPageMain);
			} else
				lcdPageChangeSubmenu(&trainDisplay);
			break;
		}
		break;
	default:
		break;
	}
}

void lcdPageChange(void (*updatePage)()) {
	digitalWrite(SD_SS_PIN, HIGH);
	(*updatePage)();
	digitalWrite(LCD_SS_PIN, HIGH);
}

void lcdPageChangeSubmenu(Train_Detail_t * td) {
	digitalWrite(SD_SS_PIN, HIGH);
	lcdPageSubMenu(td);
	digitalWrite(LCD_SS_PIN, HIGH);
}

void lcdPageMain() {
	String title, s[5];

	menuPointer = 0;
	coachPointer = 0;
	displayState = DISP_MAIN;

	//HEADER
//  title = F("Bangladesh Railway");

	//KONTEN 0
//  s[0] = trainParameter.trainInfo.trainName;
	title = trainParameter.trainInfo.trainID;
	title += " ";
	title += trainParameter.trainInfo.trainName;
	if (title.length() > 25)
		title.setCharAt(26, 0);

	//KONTEN 1
	if (trainParameter.stationInfo.state == STATION_ARRIVED) {
		s[0] = "Arrived at";
	} else if (trainParameter.stationInfo.state == STATION_TOWARD) {
		s[0] = "Going to";
	}
	s[1] = trainParameter.stationInfo.name;
	s[1] += " St.";

	//KONTEN 2
	s[2] = trainParameter.coachName;

	//KONTEN 3
	if (trainParameter.masterMode == MASTER_MODE) {
		s[3] = F("MASTER MODE");
		if (trainParameter.gpsMode == GPS_MODE)
			s[3] += F(" + GPS MODE");
	} else {
		if (dataMasterCoachId.length() && dataMasterCoachId.charAt(0) != ' ') {
			s[3] = F("Slave of ");
			s[3] += dataMasterCoachId;
		}
	}

	if (trainParameter.masterMode == SLAVE_MODE && !masterConnected) {
		title = "Error";
		s[0] = "Connection Lost";
		s[1] = "";
		s[2] = "";
		s[3] = "";
	}

	lcd.firstPage();
	do {

		lcd.setFont(u8g_font_ncenB08);
		lcd.setFontRefHeightText();
		lcd.setFontPosTop();
		lcd.drawStr(0, 0, title.c_str());

		lcd.setFont(u8g_font_5x7r);

		//KONTEN 1
		lcd.drawStr(0, 25, s[0].c_str());
		//KONTEN 2
		lcd.drawStr(0, 35, s[1].c_str());
		//KONTEN 3
		lcd.drawStr(0, 45, s[2].c_str());
		//KONTEN 5
		lcd.drawStr(0, 63, s[3].c_str());
	} while (lcd.nextPage());
}

void lcdPageOps() {

	String title, s[3], sTem;

	//HEADER
	title = F("Station:");

	//MENU CONTENT
	if (trainDisplay.stationInfo.state == STATION_ARRIVED)
		s[0] = "Arriving at";
	else if (trainDisplay.stationInfo.state == STATION_TOWARD)
		s[0] = "Going to";

	sTem = trainDisplay.stationInfo.name;
	if (sTem.length() < 21)
		s[1] = sTem;
	else {
		s[1] = sTem.substring(0, 20);
		s[1] += '.';
	}
	s[1] += " St.";

	s[2] = "OK / Cancel(Menu)";

	lcd.firstPage();
	do {
		lcd.setFont(u8g_font_ncenB08);
		lcd.setFontRefHeightText();
		lcd.setFontPosTop();
		lcd.drawStr(0, 0, title.c_str());

		lcd.setFont(u8g_font_5x7);

		lcd.drawStr(0, 30, s[0].c_str());
		lcd.drawStr(1, 45, s[1].c_str());
		lcd.drawStr(2, 60, s[2].c_str());
	} while (lcd.nextPage());
}

void lcdPageMenu() {
	String title, s[3];

	//HEADER
	title = F("Select Menu:");

	if (trainParameter.masterMode == MASTER_MODE) {
		//MENU CONTENT
		s[1] = menu_setting_en[menuPointer];

		if (menuPointer == 0) {
			s[0] = menu_setting_en[menu_setting_length - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		} else if (menuPointer >= menu_setting_length - 1) {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[0];
		} else {
			s[0] = menu_setting_en[menuPointer - 1];
			s[2] = menu_setting_en[menuPointer + 1];
		}

	}  // trainParameter.masterMode == MASTER_MODE
	else {
		lcdBacklight(1);
		s[1] = menu_setting_en[menuPointer];
	}  // else trainParameter.masterMode == MASTER_MODE

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

void lcdPageSubMenu(Train_Detail_t * td) {
	String title, s[3], tem;
	byte highlightLine = 1;
	byte a;
	byte trainPointer[3] = { 0, 0, 0 };

	//HEADER
	title = menu_setting_en[menuPointer];

	//MENU CONTENT
	switch (displayState) {
	case DISP_MENU_COACH:
		s[1] = td->coachName;
		if (td->coachName.length() < COACH_NAME_MAX_SIZE) {
			tem = td->coachName;
			for (a = 0; a < (COACH_NAME_MAX_SIZE - td->coachName.length()); a++)
				tem += ' ';

			td->coachName = tem;
		}

		highlightLine = 1;
		break;
	case DISP_MENU_TRAIN:
		trainPointer[1] = td->trainInfo.trainPosition;

		if (allTrainsName.size > 2) {
			if (trainPointer[1] == 0) {
				trainPointer[0] = allTrainsName.size - 1;
				trainPointer[2] = 1;
			} else if (trainPointer[1] == allTrainsName.size - 1) {
				trainPointer[0] = trainPointer[1] - 1;
				trainPointer[2] = 0;
			} else {
				trainPointer[0] = trainPointer[1] - 1;
				trainPointer[2] = trainPointer[1] + 1;
			}
			s[0] = String(allTrainsName.trainID[trainPointer[0]]) + ' '
					+ allTrainsName.trainName[trainPointer[0]];
			s[1] = String(allTrainsName.trainID[trainPointer[1]]) + ' '
					+ allTrainsName.trainName[trainPointer[1]];
			s[2] = String(allTrainsName.trainID[trainPointer[2]]) + ' '
					+ allTrainsName.trainName[trainPointer[2]];

		}  // allTrainsName.size > 2
		else {
			s[1] = String(allTrainsName.trainID[trainPointer[1]]) + ' '
					+ allTrainsName.trainName[trainPointer[1]];

			if (allTrainsName.size == 2) {
				if (trainPointer[1] == 0)
					trainPointer[2] = 1;
				else
					trainPointer[2] = 0;

				s[2] = String(allTrainsName.trainID[trainPointer[2]]) + ' '
						+ allTrainsName.trainName[trainPointer[2]];
			}

		}  // else allTrainsName.size > 2

		break;
	case DISP_MENU_GPS:
		s[1] = F("GPS");
		s[2] = F("MANUAL");

		if (td->gpsMode == MANUAL_MODE)
			highlightLine = 2;
		break;
	default:
		break;
	}

	lcd.firstPage();
	do {
		//HEADER
		lcd.setFont(u8g_font_ncenB08);
		lcd.setFontRefHeightText();
		lcd.setFontPosTop();
		lcd.drawStr(0, 0, title.c_str());

		if (displayState == DISP_MENU_COACH) {
			lcd.setFont(u8g_font_10x20);
			lcd.drawStr(0, 50, s[1].c_str());
			lcd.drawHLine(coachPointer * 10, 52, 10);
		} else {
			lcd.setFont(u8g_font_5x7);
			//PRINT MENU CONTENT
			for (a = 0; a < 3; a++) {
				if (highlightLine == a) {
					lcd.setDefaultForegroundColor();
					lcd.drawBox(0, 22 + (15 * a),
							lcd.getStrWidth(s[a].c_str()) + 1, 10);
					lcd.setDefaultBackgroundColor();
					lcd.drawStr(0, 30 + (15 * a), s[a].c_str());
					lcd.setDefaultForegroundColor();
				} else {
					lcd.drawStr(0, 30 + (15 * a), s[a].c_str());
				}
			}

		}
	} while (lcd.nextPage());
}

/**
 * *******************************************************************************
 * SD CARD functions
 * *******************************************************************************
 */

void sdInit() {

#if SD_DEBUG
	Serial.print(F("SD card init ..."));
#endif  //#if SD_DEBUG

	digitalWrite(LCD_SS_PIN, HIGH);
	if (!SD.begin(SD_SS_PIN)) {
#if SD_DEBUG
		Serial.println(F("failed!"));
#endif  //#if SD_DEBUG

		microSDavailable = 0;
	}  //(!SD.begin(SD_SS_PIN))
	else {
		microSDavailable = 1;
#if SD_DEBUG
		Serial.println(F("done!"));
#endif  //#if SD_DEBUG
	}
	digitalWrite(SD_SS_PIN, HIGH);
}

/**
 * *******************************************************************************
 * MASTER functions
 * *******************************************************************************
 */

void masterInit() {
#if BOARD_KONTROLLER3A
	digitalWrite(BUS_MASTER_LED_PIN, HIGH);
#elif BOARD_KONTROLLER3
	BUS_MASTER_LED_OFF();
#endif	//BOARD_KONTROLLER3A
	lcdBacklight(1);

#if DEBUG
	Serial.println(F("MASTER INIT"));
	Serial.println();
#endif  //#if DEBUG

	// find all train's name
	if (allTrainsName.size == 0)
		masterFindAllTrains();

	// select first train name as default
	masterUpdateTrain(&trainParameter, allTrainsName.trainID[0]);
#if DEBUG
	Serial.println(F("all trains name:"));
	for (int a = 0; a < allTrainsName.size; a++) {
		Serial.print(allTrainsName.trainID[a]);
		Serial.print(' ');
		Serial.println(allTrainsName.trainName[a]);
	}
	Serial.println();
#endif  //#if DEBUG

	// select first station name as default
	masterUpdateStation(&trainParameter, 0, STATION_ARRIVED);

	busSendTimer = millis() + BUS_SEND_TIMEOUT;
	digitalWrite(BUS_TX_PIN, HIGH);

#if DEBUG
	Serial.println(F("trainInfo:"));
	Serial.print(trainParameter.trainInfo.trainPosition);
	Serial.print(' ');
	Serial.print(trainParameter.trainInfo.trainID);
	Serial.print(' ');
	Serial.println(trainParameter.trainInfo.trainName);

	Serial.println();

	Serial.println(F("stationInfo:"));
	Serial.print(trainParameter.stationInfo.pos);
	Serial.print(' ');
	Serial.print(trainParameter.stationInfo.size);
	Serial.print(" \"");
	Serial.print(trainParameter.stationInfo.name);
	Serial.print("\" \"");
	Serial.print(trainParameter.stationInfo.first);
	Serial.print("\" \"");
	Serial.print(trainParameter.stationInfo.end);
	Serial.print("\" ");
	Serial.println(trainParameter.stationInfo.state);
#endif  //#if DEBUG

	masterUpdateTrainBangla();
}

void masterUpdateTrain(Train_Detail_t *td, uint16_t id) {
	String s = "";
	byte pos = 0;

	//
	td->trainInfo.trainID = id;
	masterFindTrain(id, &s, &pos);
	if (s.length() > 0) {
		td->trainInfo.trainName = s;
		td->trainInfo.trainPosition = pos;
	}
}

void masterFindTrain(uint16_t id, String *s, byte *pos) {
	byte a;
	*s = "";
	*pos = 0;

	for (a = 0; a < allTrainsName.size; a++) {
		if (allTrainsName.trainID[a] == id) {
			*s = allTrainsName.trainName[a];
			*pos = a;
			break;
		}
	}
}

void masterFindAllTrains() {
	byte a, akhir, fileCounter = 0;
	File root, fileList, theFile;
	String s, sTem, line;
	uint16_t rows = 0;
	int i;
	char c;

	wdt_reset();

#if DEBUG
	Serial.println(F("clearing allTrainsName struct"));
#endif  //#if DEBUG

	//clear Train_Filename_t struct
	for (a = 0; a < FILES_MAX_SIZE; a++) {
		allTrainsName.trainName[a] = "";
		allTrainsName.trainID[a] = 0;
	}
	allTrainsName.size = 0;

	digitalWrite(LCD_SS_PIN, HIGH);
	//find file's name for each temTrain
	root = SD.open("/");

	//find trainID & size
#if DEBUG
	Serial.println(F("Find train files:"));
#endif  //#if DEBUG
	fileCounter = 0;
	do {
		fileList = root.openNextFile();
		if (!fileList)
			break;

		if (!fileList.isDirectory()) {
			s = fileList.name();
			s.trim();
			if (s.indexOf(".PID") > 0) {

				akhir = s.indexOf(".PID");
				sTem = s.substring(0, akhir);
				i = sTem.toInt();
				if (i > 0) {
#if DEBUG
					Serial.println(s);
#endif  //#if DEBUG
					allTrainsName.trainID[fileCounter] = i;
					fileCounter++;
				}
			}
		}

	} while (fileList);
	allTrainsName.size = fileCounter;
	root.close();
	fileList.close();

	wdt_reset();

	//find train name
#if DEBUG
	Serial.println();
	Serial.println(F("Find train names:"));
#endif  //#if DEBUG

	for (a = 0; a < allTrainsName.size; a++) {
//    s = String(allTrainsName.trainID[a]) + ".PID";
		s = allTrainsName.trainID[a];
		s += ".PID";
#if DEBUG
		Serial.print(F("open "));
		Serial.println(s);
#endif  //#if DEBUG

		theFile = SD.open(s);
		if (theFile) {
			line = "";
			rows = 0;
			sTem = "";
			while (theFile.available()) {
				c = theFile.read();

				line += c;
				if (c == '\n') {
					line.trim();
					//find train's name
					if (line.indexOf("**PIDS DATA**") >= 0)
						rows = 0;
					else
						rows++;

					if (rows >= 2) {
						akhir = line.indexOf(';');
						sTem = line.substring(0, akhir);
						allTrainsName.trainName[a] = sTem;
#if DEBUG
						Serial.print(rows);
						Serial.print("->");
						Serial.print(line);
						Serial.print(' ');
						Serial.println(sTem);
#endif  //#if DEBUG
						break;
					}

					line = "";
				}
			}
		}
		theFile.close();
		sTem = "";
	}
	digitalWrite(SD_SS_PIN, HIGH);

	wdt_reset();
}

bool masterUpdateStation(Train_Detail_t *td, uint16_t pos,
		Station_State_e state) {
	char c;
	String s, sTem, lines;
	File file;
	uint16_t stationSize = 0;
	bool startStation = 0, endStation = 0;
	byte awal, akhir;

	wdt_reset();

	s = String(td->trainInfo.trainID) + ".PID";
#if SD_DEBUG
	Serial.print(F("Opening "));
	Serial.print(s);
	Serial.println(F(" file"));
	Serial.print(F("updated pos= "));
	Serial.println(pos);
#endif  //#if SD_DEBUG

	digitalWrite(LCD_SS_PIN, HIGH);
	file = SD.open(s);
	if (file) {
		lines = "";
		while (file.available()) {
			c = file.read();
			lines += c;
			if (c == '\n') {
				if (!startStation) {
					if ((lines.indexOf(",AWAL,") > 0)
							|| (lines.indexOf(",START,") > 0)) {
						startStation = 1;
						stationSize = 0;
#if SD_DEBUG
						Serial.println(F("All Station:"));
#endif  //#if SD_DEBUG

					}
				}  //(!startStation)
				else {
					if ((lines.indexOf(",AKHIR,") > 0)
							|| (lines.indexOf(",END,") > 0)) {
						endStation = 1;
#if SD_DEBUG
						Serial.println(lines);
#endif  //#if SD_DEBUG
					}
				}
				//else (!startStation)

				if (startStation) {
					// find station name
					awal = 0;
					akhir = lines.indexOf(',');
					sTem = lines.substring(awal, akhir);
#if SD_DEBUG
					Serial.print(stationSize);
					Serial.print(' ');
					Serial.println(sTem);
#endif  //#if SD_DEBUG

					if (stationSize == 0)
						td->stationInfo.first = sTem;

					if (endStation)
						td->stationInfo.end = sTem;

					// found the station
					if (stationSize == pos) {
						// station name
						td->stationInfo.name = sTem;

						// station type
						awal = akhir + 1;
						akhir = lines.indexOf(',', awal);

						//station longitude
						awal = akhir + 1;
						akhir = lines.indexOf(',', awal);
						sTem = lines.substring(awal, akhir);
						if (sTem.length() > 0)
							td->stationInfo.lon = sTem.toFloat();
						else
							td->stationInfo.lon = 0.0f;

						//station latitude
						awal = akhir + 1;
						akhir = lines.indexOf(',', awal);
						sTem = lines.substring(awal, akhir);
						if (sTem.length() > 0)
							td->stationInfo.lat = sTem.toFloat();
						else
							td->stationInfo.lat = 0.0f;

					}
					//increment station size
					stationSize++;

					if (endStation)
						break;
				}  //(startStation)

				lines = "";
			}  //(c == '\n')
		}  //file.available()

		file.close();
		digitalWrite(SD_SS_PIN, HIGH);

		// not '\n' terminated
		if (endStation == 0) {
			if ((lines.indexOf(",AKHIR,") > 0)
					|| (lines.indexOf(",END,") > 0)) {
				endStation = 1;
				awal = 0;
				akhir = lines.indexOf(',');
				sTem = lines.substring(awal, akhir);
#if SD_DEBUG
				Serial.println(F("not \\n terminated:"));
				Serial.print(stationSize);
				Serial.print(' ');
				Serial.println(sTem);
#endif  //#if SD_DEBUG
				td->stationInfo.end = sTem;

				// found the station
				if (stationSize == pos) {
					// station name
					td->stationInfo.name = sTem;

					// station type
					awal = akhir + 1;
					akhir = lines.indexOf(',', awal);

					//station longitude
					awal = akhir + 1;
					akhir = lines.indexOf(',', awal);
					sTem = lines.substring(awal, akhir);
					if (sTem.length() > 0)
						td->stationInfo.lon = sTem.toFloat();
					else
						td->stationInfo.lon = 0.0f;

					//station latitude
					awal = akhir + 1;
					akhir = lines.indexOf(',', awal);
					sTem = lines.substring(awal, akhir);
					if (sTem.length() > 0)
						td->stationInfo.lat = sTem.toFloat();
					else
						td->stationInfo.lat = 0.0f;
				}
				stationSize++;
			}
			lines = "";
		}

		// find station size
		td->stationInfo.size = stationSize;
		td->stationInfo.state = state;
#if SD_DEBUG
		Serial.print(F("Station size= "));
		Serial.println(td->stationInfo.size);
#endif  //#if SD_DEBUG

	}  //(file)
	else
		return false;

	wdt_reset();

	return true;
}

void masterChangeStationState(Train_Detail_t *td, Station_State_e state) {
	if (state == STATION_NEXT) {
		//if not end station
		if (td->stationInfo.pos < td->stationInfo.size - 1) {
			if (td->stationInfo.state == STATION_TOWARD)
				td->stationInfo.state = STATION_ARRIVED;
			else {
				td->stationInfo.pos++;
				masterUpdateStation(td, td->stationInfo.pos, STATION_TOWARD);
			}
		} else
			td->stationInfo.state = STATION_ARRIVED;

	}  //(state == STATION_NEXT)
	else if (state == STATION_PREV) {
		// if not start station
		if (td->stationInfo.pos > 0) {
			if (td->stationInfo.state == STATION_ARRIVED)
				td->stationInfo.state = STATION_TOWARD;
			else {
				td->stationInfo.pos--;
				masterUpdateStation(td, td->stationInfo.pos, STATION_ARRIVED);
			}
		}
	}  //state == STATION_PREV
}

void masterUpdateTrainBangla() {
	char c;
	String s, sTem, lines;
	uint16_t lineCount = 0;
	File file;
	uint16_t stationSize = 0;
	bool startTrainInfo = 0;
	bool startStation = 0, endStation = 0;
	byte awal;

	s = String(trainParameter.trainInfo.trainID) + ".PID";

	digitalWrite(LCD_SS_PIN, HIGH);
	file = SD.open(s);
	if (file) {
		lines = "";
		while (file.available()) {
			c = file.read();
			lines += c;
			if (c == '\n') {
				if (!startTrainInfo) {
					if (lines.indexOf("**PIDS DATA**") >= 0) {
						startTrainInfo = 1;
						lineCount = 0;
					}
				} else {
					//train.ID
					if (lineCount == 1) {
						lines.trim();
						awal = lines.indexOf(';') + 1;
						sTem = lines.substring(awal);
						trainParameter.trainInfo.idBangla = sTem;
					}
					//train.Name
					else if (lineCount == 2) {
						lines.trim();
						awal = lines.indexOf(';') + 1;
						sTem = lines.substring(awal);
						trainParameter.trainInfo.nameBangla = sTem;
					}
				}

				if (!startStation) {
					if ((lines.indexOf(",AWAL,") > 0)
							|| (lines.indexOf(",START,") > 0)) {
						startStation = 1;
						stationSize = 0;
#if SD_DEBUG
						Serial.println(F("All Station:"));
#endif  //#if SD_DEBUG

					}
				}  //(!startStation)
				else {
					if ((lines.indexOf(",AKHIR,") > 0)
							|| (lines.indexOf(",END,") > 0)) {
						endStation = 1;
#if SD_DEBUG
						Serial.println(lines);
#endif  //#if SD_DEBUG
					}
				}
				//else (!startStation)

				if (startStation) {
					// find station name
					lines.trim();
					awal = lines.indexOf(';') + 1;
					sTem = lines.substring(awal);
#if SD_DEBUG
					Serial.print(stationSize);
					Serial.print(' ');
					Serial.println(sTem);
#endif  //#if SD_DEBUG

					if (stationSize == 0)
						trainParameter.stationInfo.firstBangla = sTem;

					if (endStation)
						trainParameter.stationInfo.endBangla = sTem;

					// found the station
					if (stationSize == trainParameter.stationInfo.pos)
						trainParameter.stationInfo.nameBangla = sTem;

					//increment station size
					stationSize++;
				}  //(startStation)

				lineCount++;
				lines = "";
				if (endStation)
					break;
			}
		}
	}

	if (lines.length() > 0 && !endStation) {
		if ((lines.indexOf(",AKHIR,") > 0) || (lines.indexOf(",END,") > 0)) {
			lines.trim();
			awal = lines.indexOf(';') + 1;
			sTem = lines.substring(awal);

			trainParameter.stationInfo.endBangla = sTem;
			if (stationSize == trainParameter.stationInfo.pos)
				trainParameter.stationInfo.nameBangla = sTem;
		}
	}

	file.close();
	digitalWrite(SD_SS_PIN, HIGH);
}

/**
 * *******************************************************************************
 * SLAVE functions
 * *******************************************************************************
 */
void slaveInit(Train_Detail_t *td) {
#if BOARD_KONTROLLER3A
	digitalWrite(BUS_MASTER_LED_PIN, LOW);
#elif BOARD_KONTROLLER3
	BUS_MASTER_LED_OFF();
#endif	//BOARD_KONTROLLER3A
	lcdBacklight(1);
//clear struct
	td->masterMode = SLAVE_MODE;
	dataMasterCoachId = "";

	digitalWrite(BUS_TX_PIN, LOW);

	lcdPageChange(lcdPageMain);
}

/**
 * *******************************************************************************
 * Coach functions
 * *******************************************************************************
 */
void coachGetParameter(Train_Detail_t * td) {
	String _name = "";
	char c;
	byte a;

	for (a = 0; a < COACH_NAME_MAX_SIZE; a++) {
		c = EEPROM.read(COACH_NAME_ADDRESS + a);
		if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
			_name += c;
		else
			_name += ' ';
	}

	td->coachName = _name;
}

void coachSetParameter(Train_Detail_t * td, String newName) {
	char c;
	byte a;

// if valid then save to EEPROM
	newName.trim();
	if (newName != td->coachName) {
		for (a = 0; a < COACH_NAME_MAX_SIZE; a++) {
			c = newName.charAt(a);
			if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'))
				EEPROM.write(COACH_NAME_ADDRESS + a, c);
			else
				EEPROM.write(COACH_NAME_ADDRESS + a, ' ');
		}

		td->coachName = newName;
	}
}

/**
 * *******************************************************************************
 * Data Transfer functions
 * *******************************************************************************
 */
void dataInit() {
	pinMode(BUS_RX_PIN, OUTPUT);
	pinMode(BUS_TX_PIN, OUTPUT);
	digitalWrite(BUS_RX_PIN, LOW);
	digitalWrite(BUS_TX_PIN, LOW);
	BUS_UART.begin(9600);
#if BOARD_KONTROLLER3A
	pinMode(BUS_MASTER_LED_PIN, OUTPUT);
	digitalWrite(BUS_MASTER_LED_PIN, LOW);
	pinMode(DISP_RT_PIN, OUTPUT);
	digitalWrite(DISP_RT_PIN, HIGH);
#elif BOARD_KONTROLLER3
	BUS_MASTER_LED_INIT();
	BUS_MASTER_LED_OFF();
	DISP_RT_INIT();
	DISP_RT_TRANSMIT();
#endif	//BOARD_KONTROLLER3A

	DISP_UART.begin(38400);

	dataBus.reserve(DATA_MAX_BUFSIZE);
	dataDisp.reserve(DATA_MAX_BUFSIZE);
}

void dataHandler() {
	char c;
	bool sCompleted = 0;
	static uint32_t masterFoundTimer = CONN_LOST_TIMEOUT;

	if (BUS_UART.available()) {
		c = BUS_UART.read();
		if (trainParameter.masterMode == SLAVE_MODE) {

			if (c == '\n')
				sCompleted = 1;
			else if (c == '$')
				dataBus = "";

			dataBus += c;
#if DATA_DEBUG
			Serial.write(c);
#endif  //#if DEBUG

			if (sCompleted) {
#if DATA_DEBUG
				Serial.print(F("data "));
				Serial.print(dataBus.length());
				Serial.println(F("B found!"));
#endif  //#if DEBUG

				if (validateCRC(&dataBus)) {
					dataBusParsing();
#if DATA_DEBUG
					Serial.println(F("data bus valid received"));
#endif  //#if DATA_DEBUG

					masterFoundTimer = millis() + CONN_LOST_TIMEOUT;
				}

				dataBus = "";
			}  //(sCompleted)
		}  //(trainParameter.masterMode == SLAVE_MODE)
	} //if (BUS_UART.available())

	if (trainParameter.masterMode == MASTER_MODE) {
		dataBusMasterSend();
		dataDispSend();
	} else {
		if (millis() < masterFoundTimer) {
			dataDispSend();
			masterConnected = true;
			lcdBacklight(1);
		} else {
			dataMasterCoachId = "";
			masterConnected = false;
			lcdBacklight(0);
		}
	}
}

void dataBusMasterSend() {
	static bool transmission = 0;
	static uint16_t transmitPointer = 0;

	if (millis() >= busSendTimer) {
		busSendTimer = millis() + BUS_SEND_TIMEOUT;
		if (dataBusCreate(&dataBus)) {
			//Serial.print(dataBus);
			transmission = 1;
			transmitPointer = 0;
#if DATA_DEBUG
			Serial.println(bitRead(PORTD, PORTD4));
			Serial.println(F("Send Data Bus"));
			Serial.println(dataBus);
#endif  //#if DATA_DEBUG

		}
	}

	if (transmission) {
		//wait till tx buffer at least 60B free
		if (BUS_UART.availableForWrite() > 60) {
			for (byte a = 0; a < 60; a++) {
				if (transmitPointer < dataBus.length())
					BUS_UART.write(dataBus.charAt(transmitPointer++));
				else {
					// transmission ended
					transmission = 0;
					break;
				}
			}
		}
	}  // (transmission)

}

bool dataBusCreate(String *s) {
	static byte counter = 0;
	String _coachName = trainParameter.coachName;
	_coachName.trim();

	if (counter % 2 == 0) {
		*s = "$";
		*s += counter;
		*s += ',';
		*s += _coachName;
		*s += ',';
		*s += trainParameter.trainInfo.trainID;
		*s += ',';
		*s += trainParameter.trainInfo.trainName;
		*s += ',';
		*s += trainParameter.stationInfo.name;
		*s += ',';
		*s += trainParameter.stationInfo.state;
		*s += ',';
		*s += trainParameter.stationInfo.first;
		*s += ',';
		*s += trainParameter.stationInfo.end;
		*s += '*';
	} else {
		*s = "$";
		*s += counter;
		*s += ',';
		*s += _coachName;
		*s += ',';
		*s += trainParameter.trainInfo.idBangla;
		*s += ',';
		*s += trainParameter.trainInfo.nameBangla;
		*s += ',';
		*s += trainParameter.stationInfo.nameBangla;
		*s += ',';
		*s += trainParameter.stationInfo.state;
		*s += ',';
		*s += trainParameter.stationInfo.firstBangla;
		*s += ',';
		*s += trainParameter.stationInfo.endBangla;
		*s += '*';

	}

	counter++;

	if (addCRC(s))
		return true;
	else
		return false;
}

void dataBusParsing() {
	uint16_t awal, akhir;
	String tem;
	byte counter = 0;

	akhir = dataBus.indexOf('$');

#if DATA_DEBUG
	Serial.flush();
	Serial.println(dataBus);
	Serial.flush();
	Serial.println(F("Parsing data:"));
#endif  //#if DATA_DEBUG

//counter
	awal = akhir + 1;
	akhir = dataBus.indexOf(',', awal);
	tem = dataBus.substring(awal, akhir);
	counter = tem.toInt();

// master's coach id
	awal = akhir + 1;
	akhir = dataBus.indexOf(',', awal);
	tem = dataBus.substring(awal, akhir);
	dataMasterCoachId = tem;
#if DATA_DEBUG
	Serial.print(tem);
	Serial.print(F(" = "));
	Serial.println(dataMasterCoachId);
#endif  //#if DATA_DEBUG

// train Number
	awal = akhir + 1;
	akhir = dataBus.indexOf(',', awal);
	tem = dataBus.substring(awal, akhir);
	if (counter % 2 == 0)
		trainParameter.trainInfo.trainID = tem.toInt();
	else
		trainParameter.trainInfo.idBangla = tem;
#if DATA_DEBUG
	Serial.print(tem);
	Serial.print(F(" = "));
	Serial.println(trainParameter.trainInfo.trainID);
#endif  //#if DATA_DEBUG

// train Name
	awal = akhir + 1;
	akhir = dataBus.indexOf(',', awal);
	tem = dataBus.substring(awal, akhir);
	if (counter % 2 == 0)
		trainParameter.trainInfo.trainName = tem;
	else
		trainParameter.trainInfo.nameBangla = tem;
#if DATA_DEBUG
	Serial.print(tem);
	Serial.print(F(" = "));
	Serial.println(trainParameter.trainInfo.trainName);
#endif  //#if DATA_DEBUG

// Station Name
	awal = akhir + 1;
	akhir = dataBus.indexOf(',', awal);
	tem = dataBus.substring(awal, akhir);
	if (counter % 2 == 0)
		trainParameter.stationInfo.name = tem;
	else
		trainParameter.stationInfo.nameBangla = tem;
#if DATA_DEBUG
	Serial.print(tem);
	Serial.print(F(" = "));
	Serial.println(trainParameter.stationInfo.name);
#endif  //#if DATA_DEBUG

// Station state
	awal = akhir + 1;
	akhir = dataBus.indexOf(',', awal);
	tem = dataBus.substring(awal, akhir);
	if (tem.charAt(0) == '1')
		trainParameter.stationInfo.state = STATION_TOWARD;
	else
		trainParameter.stationInfo.state = STATION_ARRIVED;
#if DATA_DEBUG
	Serial.print(tem);
	Serial.print(F(" = "));
	Serial.println(trainParameter.stationInfo.state);
#endif  //#if DATA_DEBUG

// Station First
	awal = akhir + 1;
	akhir = dataBus.indexOf(',', awal);
	tem = dataBus.substring(awal, akhir);
	if (counter % 2 == 0)
		trainParameter.stationInfo.first = tem;
	else
		trainParameter.stationInfo.firstBangla = tem;
#if DATA_DEBUG
	Serial.print(tem);
	Serial.print(F(" = "));
	Serial.println(trainParameter.stationInfo.first);
#endif  //#if DATA_DEBUG

// Station End
	awal = akhir + 1;
	akhir = dataBus.indexOf('*', awal);
	tem = dataBus.substring(awal, akhir);
	if (counter % 2 == 0)
		trainParameter.stationInfo.end = tem;
	else
		trainParameter.stationInfo.endBangla = tem;
#if DATA_DEBUG
	Serial.print(tem);
	Serial.print(F(" = "));
	Serial.println(trainParameter.stationInfo.end);
#endif  //#if DATA_DEBUG

	if (displayState == DISP_MAIN)
		lcdPageChange(lcdPageMain);

}

void dataDispSend() {
	static bool transmission = 0;
	static uint16_t transmitPointer = 0;

	if (millis() >= dispSendTimer) {
		dispSendTimer = millis() + DISP_SEND_TIMEOUT;

		if (dataDispCreate(&dataDisp)) {
#if DATA_DEBUG
			Serial.println(F("==data to Display=="));
			Serial.println(dataDisp);
#endif
			transmission = 1;
			transmitPointer = 0;
		}

	}  //(millis() >= sendTimer)

	if (transmission) {
		//wait till tx buffer at least 60B free
		if (DISP_UART.availableForWrite() > 60) {
			for (byte a = 0; a < 60; a++) {
				if (transmitPointer < dataDisp.length())
					DISP_UART.write(dataDisp.charAt(transmitPointer++));
				else {
					// transmission ended
					transmission = 0;
					break;
				}
			}
		}
	}  // (transmission)
}

bool dataDispCreate(String *s) {
	static byte counter = 0;
	String tem;

	if (counter % 2 == 0) {

		*s = "$";
		*s += counter;
		*s += ',';
		*s += trainParameter.trainInfo.trainID;
		*s += ',';
		*s += trainParameter.trainInfo.trainName;
		*s += ',';
		*s += trainParameter.stationInfo.name;
		*s += ',';
		*s += trainParameter.stationInfo.state;
		*s += ',';
		*s += trainParameter.stationInfo.first;
		*s += ',';
		*s += trainParameter.stationInfo.end;
		*s += ',';
		tem = trainParameter.coachName;
		tem.trim();
		*s += tem;
		*s += '*';
	} else {
		*s = "$";
		*s += counter;
		*s += ',';
		*s += trainParameter.trainInfo.idBangla;
		*s += ',';
		*s += trainParameter.trainInfo.nameBangla;
		*s += ',';
		*s += trainParameter.stationInfo.nameBangla;
		*s += ',';
		*s += trainParameter.stationInfo.state;
		*s += ',';
		*s += trainParameter.stationInfo.firstBangla;
		*s += ',';
		*s += trainParameter.stationInfo.endBangla;
		*s += ',';
		tem = trainParameter.coachName;
		tem.trim();
		*s += tem;
		*s += '*';

	}

#if DATA_DEBUG
	Serial.println(F("==before CRC added=="));
	Serial.println(*s);
#endif

//  counter += 2;
	counter++;

	if (addCRC(s))
		return true;
	else
		return false;

}

bool validateCRC(String *s) {
	bool ret = false;
	byte crcVal = 0;
	uint16_t i, awal, akhir;
	String crcIn, crcCreated;

	awal = s->indexOf('$') + 1;
	akhir = s->indexOf('*');

	if (akhir > awal) {
		for (i = awal; i < akhir; i++)
			crcVal ^= s->charAt(i);

		awal = s->indexOf('*') + 1;
		akhir = s->indexOf('\n');
		crcIn = s->substring(awal, akhir);

		crcCreated = String(crcVal >> 4, HEX);
		crcCreated += String(crcVal & 0xF, HEX);

#if DATA_DEBUG
		Serial.print(crcCreated);
		Serial.print(" = ");
		Serial.println(crcIn);
#endif  //#if DEBUG

		if (crcCreated == crcIn)
			ret = true;
	}  //(akhir > awal)

	return ret;
}

bool addCRC(String *s) {
	bool ret = false;
	byte crcVal = 0;
	uint16_t i, awal, akhir;

	if (s->length() <= DATA_MAX_BUFSIZE) {
		awal = s->indexOf('$') + 1;
		akhir = s->indexOf('*');

		if (akhir > awal) {
			crcVal = 0;
			for (i = awal; i < akhir; i++)
				crcVal ^= byte(s->charAt(i));

			*s += String(crcVal >> 4, HEX);
			*s += String(crcVal & 0xF, HEX);
			*s += '\n';

			ret = true;
		}  //(akhir > awal)
	}  //(s->length() <= DATA_MAX_BUFSIZE)

	return ret;
}

/**
 * *******************************************************************************
 * GPS functions
 * *******************************************************************************
 */
void gpsInit() {
	Serial2.begin(GPS_BAUD_RATE);
}

void gpsHandler() {
	char c;

	if (Serial2.available()) {
		c = Serial2.read();

		if (trainParameter.masterMode == MASTER_MODE
				&& trainParameter.gpsMode == GPS_MODE
				&& displayState == DISP_MAIN) {
			if (gps.encode(c)) {
				gpsUpdate();
			}
		}
	}
}

void gpsUpdate() {
	uint32_t distance = 0;

	if (gps.location.isUpdated()) {
		// find distance
		distance = gps.distanceBetween(gps.location.lat(), gps.location.lng(),
				trainParameter.stationInfo.lat, trainParameter.stationInfo.lon);

#if GPS_DEBUG
		Serial.println(distance);
#endif	//GPS_DEBUG
		if (distance <= GPS_STATION_RADIUS)
			gpsTrainInStation = 1;

		if (trainParameter.stationInfo.state == STATION_ARRIVED) {
			if (distance >= GPS_STATION_DISTANCE_LEAVE
					&& gpsTrainInStation == 1) {
				masterChangeStationState(&trainParameter, STATION_NEXT);
				masterUpdateTrainBangla();
				gpsTrainInStation = 0;
				lcdPageChange(lcdPageMain);
#if GPS_DEBUG
				Serial.print(F("Going To "));
				Serial.println(trainParameter.stationInfo.name);
#endif	//GPS_DEBUG
			}
		} else if (trainParameter.stationInfo.state == STATION_TOWARD) {
			if (distance <= GPS_STATION_DISTANCE_ENTER) {
				masterChangeStationState(&trainParameter, STATION_NEXT);
				masterUpdateTrainBangla();
				lcdPageChange(lcdPageMain);
#if GPS_DEBUG
				Serial.print(F("Arrived At "));
				Serial.println(trainParameter.stationInfo.name);
#endif	//GPS_DEBUG
			}
		}

	}

}

/**
 * *******************************************************************************
 * Button functions
 * *******************************************************************************
 */

void btnInit() {
	byte a;

#if BUTTON_DEBUG
	Serial.print(F("Button init..."));
#endif  //#if BUTTON_DEBUG

	for (a = 0; a < 4; a++)
		pinMode(button[a], INPUT_PULLUP);

	pinMode(MASTER_PIN, INPUT_PULLUP);

#if BUTTON_DEBUG
	Serial.println(F("done!"));
#endif  //#if BUTTON_DEBUG

}

void btnHandler() {
	static uint32_t _update = 0;
	static byte _prevButtonVal = 0;
	byte buttonVal = 0;

	if (_update == 0)
		buttonVal = btnRead();

	if (buttonVal != _prevButtonVal || _update) {
		if (_update == 0) {
			_update = millis() + DEBOUNCE_DELAY;
		}  //(_update == 0)
		else {
			if (millis() >= _update) {
				buttonVal = btnRead();
				if (buttonVal != _prevButtonVal) {
					_prevButtonVal = buttonVal;

					btnUpdate(buttonVal);
				}  //(buttonVal != _prevButtonVal)
				_update = 0;
			}  //(millis() >= _update)
		}
	}  //(buttonVal != _prevButtonVal)
}

void btnUpdate(byte button) {
	bool masterButtonChanged = 0;

	/* Check master button input */
	if (bitRead(button, 4)) {
		// previous master mode is slave
		if (microSDavailable == 1 && trainParameter.masterMode == SLAVE_MODE) {
			trainParameter.masterMode = MASTER_MODE;
#if DEBUG
			Serial.println(F("Change to MASTER_MODE"));
#endif  //#if DEBUG

			//update trainDetail struct
			masterInit();
			masterButtonChanged = 1;
		}
	} else {
		// previous master mode is master
		if (trainParameter.masterMode == MASTER_MODE) {
			trainParameter.masterMode = SLAVE_MODE;
#if DEBUG
			Serial.println(F("Change to SLAVE_MODE"));
#endif  //#if DEBUG

			slaveInit(&trainParameter);
			masterButtonChanged = 1;
		}
	}

	if (masterButtonChanged) {
		///updating lcd display when in main page only
		menuPointer = 0;
		displayState = DISP_MAIN;
		lcdPageChange(lcdPageMain);
	}  //(masterButtonChanged)

	/* Check state button input */
	button &= 0x0F;
	if ((button == BTN_SELECT) || (button == BTN_BACK) || (button == BTN_NEXT)
			|| (button == BTN_PREV)) {
		lcdPageUpdate(button);

		idleTimer = millis() + IDLE_TIMEOUT;
	}

#if BUTTON_DEBUG
	Serial.println(button, BIN);
#endif  //#if BUTTON_DEBUG

}

byte btnRead() {
	byte ret = 0, a;

	for (a = 0; a < 4; a++)
		bitWrite(ret, a, !digitalRead(button[a]));

	bitWrite(ret, 4, !digitalRead(MASTER_PIN));

	return ret;
}

/**
 * *******************************************************************************
 * WDT functions
 * *******************************************************************************
 */
//delay with wdt enable
void delayWdt(unsigned int _delay) {
	wdt_reset();
	if (_delay <= WDT_RESET_TIME) {
		delay(_delay);
	} else {
		while (_delay > WDT_RESET_TIME) {
			delay(WDT_RESET_TIME);
			wdt_reset();
			_delay -= WDT_RESET_TIME;
		}
		delay(_delay);
	}
	wdt_reset();
}

