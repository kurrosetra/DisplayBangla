#include "Arduino.h"
#include <SPI.h>
#include <SD.h>
#include "U8glib.h"

/**
 * PINOUT
 */
#define BOX_TEST					0

#if BOX_OLD
const byte MASTER_PIN = A4;
const byte LCD_BACKLIGHT_PIN = A7;
const byte LCD_SS_PIN = 48;
const byte SD_SS_PIN = 53;
const byte button[4] =
{	A1/*RIGHT / SELECT*/, A3/*LEFT / BACK*/, A0 /*DOWN / NEXT*/, A2 /*UP / PREV*/};

#define BUS_UART		Serial3
#define DISP_UART		Serial2

#define BUS_RT_INIT()				bitSet(DDRF,DDF6)
#define BUS_RT_TRANSMIT()		bitSet(PORTF,PORTF6)
#define BUS_RT_RECEIVE()		bitClear(PORTF,PORTF6)

#define DISP_RT_INIT()			;
#define DISP_RT_TRANSMIT()	;
#define DISP_RT_RECEIVE()		;

#else
const byte MASTER_PIN = A7;
const byte LCD_BACKLIGHT_PIN = 6;
const byte LCD_SS_PIN = 4;
const byte SD_SS_PIN = 7;
const byte button[4] =
		{ A3/*RIGHT / SELECT*/, A6/*LEFT / BACK*/, A4 /*DOWN / NEXT*/, A5 /*UP / PREV*/};

#define BUS_UART		Serial1
#define DISP_UART		Serial3

#define BUS_RT_INIT()				bitSet(DDRD,DDD4)
#define BUS_RT_TRANSMIT()		bitSet(PORTD,PORTD4)
#define BUS_RT_RECEIVE()		bitClear(PORTD,PORTD4)

#define DISP_RT_INIT()			bitSet(DDRJ,DDJ2)
#define DISP_RT_TRANSMIT()	bitSet(PORTJ,PORTJ2)
#define DISP_RT_RECEIVE()		bitClear(PORTJ,PORTJ2)

#endif	//#if else BOX_OLD

const uint32_t DEBOUNCE_DELAY = 100;

U8GLIB_ST7920_128X64_4X lcd(LCD_SS_PIN);

void setup()
{

	Serial.begin(115200);
	Serial.println(F("Display Bangla test suite!"));
	Serial.println(F("=========================="));
	Serial.println(F("1. Button Test"));
	Serial.println(F("2. LCD Test"));
	Serial.println(F("3. SD Card Test"));
	Serial.println(F("4. Bus Slave Test"));
	Serial.println(F("5. Bus Master Test"));
	Serial.println(F("6. Display Command Test"));
	Serial.println(F("7. BUS-SLAVE & DISPLAY COMMAND test"));
	Serial.println(F("=========================="));

	buttonInit();
	LCD_SD_Init();
	busInit();
	dispInit();
}

void loop()
{
	char c;

	if (Serial.available()) {
		c = Serial.read();
		switch (c)
		{
			case '1':
				Serial.println(F("Start button test"));
				buttonTest();
				Serial.println(F("End button test"));
				break;
			case '2':
				Serial.println(F("Start LCD test"));
				lcdTest();
				Serial.println(F("End LCD test"));
				break;
			case '3':
				Serial.println(F("Start SD Card test"));
				sdTest();
				Serial.println(F("End SD Card test"));
				break;
			case '4':
				Serial.println(F("Start BUS-SLAVE test"));
				busSlaveTest();
				Serial.println(F("End BUS-SLAVE test"));
				break;
			case '5':
				Serial.println(F("Start BUS-MASTER test"));
				busMasterTest();
				Serial.println(F("End BUS-MASTER test"));
				break;
			case '6':
				Serial.println(F("Start DISPLAY COMMAND test"));
				dispTest();
				Serial.println(F("End DISPLAY COMMAND test"));
				break;
			case '7':
				Serial.println(F("Start BUS-MASTER & DISPLAY COMMAND test"));
				busSlaveDispTest();
				Serial.println(F("End BUS-MASTER & DISPLAY COMMAND test"));
				break;
		}
	}
}

void buttonInit()
{
	for ( byte a = 0; a < 4; a++ ) {
		pinMode(button[a], INPUT_PULLUP);
	}

	pinMode(MASTER_PIN, INPUT_PULLUP);
}

byte buttonRead()
{
	byte ret = 0, a = 0;

	for ( a = 0; a < 4; a++ )
		bitWrite(ret, a, !digitalRead(button[a]));

	bitWrite(ret, 4, !digitalRead(MASTER_PIN));

	return ret;
}

void buttonTest()
{
	bool exit = 0;
	byte _prevButtonVal = 0;
	byte buttonVal = 0;

	do {
		buttonVal = buttonRead();

		if (buttonVal != _prevButtonVal) {
			delay(DEBOUNCE_DELAY);
			buttonVal = buttonRead();
			if (buttonVal != _prevButtonVal) {
				_prevButtonVal = buttonVal;

				Serial.print(F("button= 0b"));
				Serial.println(buttonVal, BIN);
			}
		}

		exit = exitTestFunction();
	} while (!exit);

}

void LCD_SD_Init()
{
	pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
	digitalWrite(LCD_BACKLIGHT_PIN, HIGH);

	pinMode(LCD_SS_PIN, OUTPUT);
	pinMode(SD_SS_PIN, OUTPUT);

	digitalWrite(LCD_SS_PIN, HIGH);
	digitalWrite(SD_SS_PIN, HIGH);

	pinMode(53, OUTPUT);
	digitalWrite(53, HIGH);
}

void lcdTest()
{
	bool exit = 0;

	String title, s[4];
	u8g_uint_t w, d;
	uint32_t updateTimer = 0;

	digitalWrite(LCD_SS_PIN, LOW);
	digitalWrite(SD_SS_PIN, HIGH);

	do {
		if (millis() > updateTimer) {
			updateTimer = millis() + 1000;

			title = "LCD TEST";
			s[0] = "timestamp= ";
			s[1] = millis() / 60000;
			s[1] += " minute";
			s[2] = (millis() % 60000) / 1000;
			s[2] += " seconds";

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

		exit = exitTestFunction();
	} while (!exit);
}

void sdTest()
{
	String s;

	Serial.print(F("SD card init ..."));

	digitalWrite(LCD_SS_PIN, HIGH);
	if (!SD.begin(SD_SS_PIN))
		Serial.println(F("failed!"));
	else
		Serial.println(F("done!"));

	File root, filename;

	root = SD.open("/");
	Serial.println(F("list of filename:"));
	do {
		filename = root.openNextFile();
		if (!filename.isDirectory()) {
			s = filename.name();
			Serial.println(s);
		}
	} while (filename);

	filename.close();
	root.close();
	Serial.println(F("===end of filename list==="));

}

void busInit()
{
	BUS_RT_INIT();
	BUS_UART.begin(9600);
}

void busSlaveTest()
{
	bool exit = 0;

	BUS_RT_RECEIVE();

	do {
		if (BUS_UART.available())
			Serial.write(BUS_UART.read());

		exit = exitTestFunction();
	} while (!exit);

}

void busMasterTest()
{
	bool exit = 0;
	char c;

	BUS_RT_TRANSMIT();

	do {
		if (Serial.available()) {
			c = Serial.read();

			if (c == '$')
				exit = 1;
			else
				BUS_UART.write(c);
		}
	} while (!exit);
}

void dispInit()
{
	DISP_RT_INIT();
	DISP_RT_RECEIVE();
	DISP_UART.begin(9600);
}

void dispTest()
{

	bool exit = 0;

	DISP_RT_RECEIVE();

	do {
		if (DISP_UART.available())
			Serial.write(DISP_UART.read());

		exit = exitTestFunction();
	} while (!exit);

}

void busSlaveDispTest()
{
	bool exit = 0;
	char c;

	BUS_RT_RECEIVE();
	DISP_RT_TRANSMIT();

	do {
		if (BUS_UART.available())
			Serial.write(BUS_UART.read());

		if (Serial.available()) {
			c = Serial.read();
			if (c == '$') {
				exit = 1;
				break;
			}
			else
				DISP_UART.write(c);
		}

	} while (!exit);
}

bool exitTestFunction()
{
	bool ret = 0;
	char c;

	if (Serial.available()) {
		c = Serial.read();
		if (c == '$')
			ret = 1;
	}

	return ret;
}
