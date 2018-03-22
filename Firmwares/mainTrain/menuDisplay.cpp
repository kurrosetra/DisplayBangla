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
	trainDetail.menuLang=ENGLISH_LANG;
}

void menuDisplay::backlight(bool onf)
{
	digitalWrite(_lcd_backligth_pin, onf);
}

void menuDisplay::pageUpdate()
{
	lcd->firstPage();
	do {
		pageMenu(0);
	} while (lcd->nextPage());
//	if (displayState != currentDisplayState) {
//		lcd->firstPage();
//		do {
//			switch (displayState)
//			{
//			case DISP_IDLE:
//				if (currentDisplayState != DISP_MAIN)
//					pageMain();
//				backlight(LOW);
//				break;
//			case DISP_MAIN:
//				backlight(HIGH);
//				pageMain();
//				currentMenuPointer = 0;
//				menuPointer = 0;
//				break;
//			case DISP_MENU:
//				if (menuPointer != currentMenuPointer) {
//					pageMenu(menuPointer);
//					currentMenuPointer = menuPointer;
//				}	//(menuPointer != currentMenuPointer)
//				break;
//			case DISP_MENU_MENU_LANGUAGE:
//				break;
//			case DISP_MENU_DISPLAY_LANGUAGE:
//				break;
//			case DISP_MENU_TRAIN_NAME:
//				break;
//			case DISP_MENU_TRAIN_ROUTE:
//				break;
//			case DISP_MENU_CURRENT_STATION:
//				break;
//			case DISP_MENU_MASTER_MODE:
//				break;
//			case DISP_MENU_GPS_MODE:
//				break;
//			case DISP_MENU_COACH:
//				break;
//			case DISP_MENU_COACH_NUMBER:
//				break;
//			case DISP_MENU_CHANGE_REQ:
//				break;
//			case DISP_MENU_SYSTEM_INFO:
//				break;
//			default:
//				break;
//			}
//		} while (lcd->nextPage());
//
//		currentDisplayState = displayState;
//	}	//(displayState != currentDisplayState)

}

void menuDisplay::setMenuPointer(byte pos)
{
	menuPointer = pos;
}

byte menuDisplay::getMenuPointer()
{
	return currentMenuPointer;
}

void menuDisplay::pageMain()
{
	String s;
	u8g_uint_t w, d;

	//HEADER
	w = lcd->getWidth();
	lcd->setFont(u8g_font_ncenB08);
	lcd->setFontRefHeightText();
	lcd->setFontPosTop();
	if (trainDetail.menuLang == ENGLISH_LANG)
		s = F("Bangladesh Railway");
	else
		s = F("~Bangladesh Railway~");
	d = (w - lcd->getStrWidth(s.c_str())) / 2;
	lcd->drawStr(d, 0, s.c_str());

	//SUB-HEADER
	lcd->setFont(u8g_font_5x7r);
	if (trainDetail.menuLang == ENGLISH_LANG)
		s = F("Display Controller");
	else
		s = F("~Display Controller~");
	d = (w - lcd->getStrWidth(s.c_str())) / 2;
	lcd->drawStr(d, 25, s.c_str());
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
	d = (w - lcd->getStrWidth(s.c_str())) / 2;
	lcd->drawStr(d, 63, s.c_str());
}

void menuDisplay::pageMenu(byte posMenu)
{
	String s[3];

	//HEADER
	lcd->setFont(u8g_font_ncenB08);
	lcd->setFontRefHeightText();
	lcd->setFontPosTop();
	if (trainDetail.menuLang == ENGLISH_LANG)
		s[0] = F("Select Menu:");
	else
		s[0] = F("~Select Menu~");
	lcd->drawStr(0, 0, s[0].c_str());

	//MENU CONTENT
	if (trainDetail.menuLang == ENGLISH_LANG)
		s[1] = menu_setting_en[posMenu];
	else
		s[1] = menu_setting_bl[posMenu];

	if (posMenu == 0) {
		if (trainDetail.menuLang == ENGLISH_LANG) {
			s[0] = menu_setting_en[menu_setting_length - 1];
			s[2] = menu_setting_en[posMenu + 1];
		}
		else {
			s[0] = menu_setting_bl[menu_setting_length - 1];
			s[2] = menu_setting_bl[posMenu + 1];
		}
	}
	else if (posMenu >= menu_setting_length - 1) {
		if (trainDetail.menuLang == ENGLISH_LANG) {
			s[0] = menu_setting_en[posMenu - 1];
			s[2] = menu_setting_en[0];
		}
		else {
			s[0] = menu_setting_bl[posMenu - 1];
			s[2] = menu_setting_bl[0];
		}
	}

	lcd->setFont(u8g_font_5x7);
	lcd->drawStr(0, 30, s[0].c_str());
	lcd->setDefaultForegroundColor();
	lcd->drawBox(0, 37, lcd->getStrWidth(s[1].c_str()) + 1, 10);
	lcd->setDefaultBackgroundColor();
	lcd->drawStr(1, 45, s[1].c_str());
	lcd->setDefaultForegroundColor();
	lcd->drawStr(2, 60, s[2].c_str());

//	//String 1
//	if (selection == 1) {
//		lcd->setFontMode(1);
//		lcd->setDrawColor(2);
//		i = lcd->getStrWidth(opt1.c_str());
//		lcd->drawBox(0, 22, i + 1, 10);
//	}
//	lcd->drawStr(0, 30, opt1.c_str());
}

//void menuDisplay::pageSubMenu(String title, String opt1, String opt2,
//		String opt3, byte selection)
//{
//	int i = 0;
//
//	lcd->clearBuffer();
//
//	//HEADER
//	lcd->setFont(u8g2_font_ncenB08_tf);
//	lcd->drawStr(0, 10, title.c_str());
//	lcd->setFont(u8g2_font_5x7_tf);
//
//	//String 1
//	if (selection == 1) {
//		lcd->setFontMode(1);
//		lcd->setDrawColor(2);
//		i = lcd->getStrWidth(opt1.c_str());
//		lcd->drawBox(0, 22, i + 1, 10);
//	}
//	lcd->drawStr(0, 30, opt1.c_str());
//
//	//String 2
//	if (selection == 2) {
//		lcd->setFontMode(1);
//		lcd->setDrawColor(2);
//		i = lcd->getStrWidth(opt2.c_str());
//		lcd->drawBox(0, 37, i + 1, 10);
//	}
//	lcd->drawStr(0, 45, opt2.c_str());
//
//	//String 3
//	if (selection == 3) {
//		lcd->setFontMode(1);
//		lcd->setDrawColor(2);
//		i = lcd->getStrWidth(opt3.c_str());
//		lcd->drawBox(0, 52, i + 1, 10);
//	}
//	lcd->drawStr(0, 60, opt3.c_str());
//
//	lcd->sendBuffer();
//}
