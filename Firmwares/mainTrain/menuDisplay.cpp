/*
 * menuDisplay.cpp
 *
 *  Created on: Mar 19, 2018
 *      Author: miftakur
 */

#include "menuDisplay.h"

void menuDisplay::init()
{
	pinMode(_lcd_backligth_pin, OUTPUT);
	backlight(1);
	lcd->begin();
	lcd->setFont(u8g2_font_ncenB10_tr);
}

void menuDisplay::backlight(bool onf)
{
	digitalWrite(_lcd_backligth_pin, onf);
}

void menuDisplay::pageMain(byte master, byte gps)
{
	String s;

	lcd->clearBuffer();

	//HEADER
	lcd->setFont(u8g2_font_ncenB08_tf);
	s = F("Bangladesh Railway");
	lcd->drawStr(0, 10, s.c_str());
	//SUB-HEADER
	lcd->setFont(u8g2_font_5x7_tr);
	s = F("Display Controller");
	lcd->drawStr(0, 25, s.c_str());
	//KONTEN 1
	//KONTEN 2
	if (master)
		s = F("Master");
	else
		s = F("Slave");
	s += ';';
	if (gps)
		s += F("GPS");
	else
		s += F("Manual");
	lcd->setFont(u8g2_font_5x7_tr);
	lcd->drawStr(0, 64, s.c_str());

	lcd->sendBuffer();
}

void menuDisplay::pageSubMenu(String title, String opt1, String opt2,
		byte selection)
{
	int i;

	lcd->clearBuffer();

	//HEADER
	lcd->setFont(u8g2_font_ncenB08_tf);
	lcd->drawStr(0, 10, title.c_str());
	lcd->setFont(u8g2_font_5x7_tf);

	//String 1
	if (selection) {
		lcd->setFontMode(1);
		lcd->setDrawColor(2);
		i = lcd->getStrWidth(opt1.c_str());
		lcd->drawBox(0, 37, i + 1, 10);
	}
	lcd->drawStr(0, 45, opt1.c_str());

	//String 2
	if (!selection) {
		lcd->setFontMode(1);
		lcd->setDrawColor(2);
		i = lcd->getStrWidth(opt2.c_str());
		lcd->drawBox(0, 52, i + 1, 10);
	}
	lcd->drawStr(0, 60, opt2.c_str());

	lcd->sendBuffer();
}

void menuDisplay::pageSubMenu(String title, String opt1, String opt2,
		String opt3, byte selection)
{
	int i = 0;

	lcd->clearBuffer();

	//HEADER
	lcd->setFont(u8g2_font_ncenB08_tf);
	lcd->drawStr(0, 10, title.c_str());
	lcd->setFont(u8g2_font_5x7_tf);

	//String 1
	if (selection == 1) {
		lcd->setFontMode(1);
		lcd->setDrawColor(2);
		i = lcd->getStrWidth(opt1.c_str());
		lcd->drawBox(0, 22, i + 1, 10);
	}
	lcd->drawStr(0, 30, opt1.c_str());

	//String 2
	if (selection == 2) {
		lcd->setFontMode(1);
		lcd->setDrawColor(2);
		i = lcd->getStrWidth(opt2.c_str());
		lcd->drawBox(0, 37, i + 1, 10);
	}
	lcd->drawStr(0, 45, opt2.c_str());

	//String 3
	if (selection == 3) {
		lcd->setFontMode(1);
		lcd->setDrawColor(2);
		i = lcd->getStrWidth(opt3.c_str());
		lcd->drawBox(0, 52, i + 1, 10);
	}
	lcd->drawStr(0, 60, opt3.c_str());

	lcd->sendBuffer();
}
