/*
 * menuDisplay.h
 *
 *  Created on: Mar 19, 2018
 *      Author: miftakur
 */

#ifndef MENUDISPLAY_H_
#define MENUDISPLAY_H_

#include <SPI.h>
#include "U8glib.h"

class menuDisplay
{
public:
	menuDisplay(byte ss, byte backlight)
	{
		lcd = new U8GLIB_ST7920_128X64_4X(ss);
		_lcd_backligth_pin = backlight;
	}

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
		DISP_MENU_COACH_NUMBER,
		DISP_MENU_CHANGE_REQ
	};
	byte displayState = DISP_MAIN;
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

	static const byte MAX_COACH_LIST_ID = 20;
	struct Coach_List
	{
		char *name;
		byte howmany;
	};

	struct Train_Info
	{
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
		Coach_List coachList[MAX_COACH_LIST_ID];  // all available coach id and how many
		bool masterMode;	// master/slave mode (slave=0, master=1)
		bool gpsMode;		// gps/manual mode (manual=0, gps=1)
	};
	Train_Detail trainDetail;
	byte menuPointer = 0;

	void init();
	void backlight(bool onf);

	void pageUpdate();
	void setMenuPointer(byte pos);
	byte getMenuPointer();

private:
	U8GLIB_ST7920_128X64_4X *lcd;

	byte _lcd_backligth_pin;
	static const byte menu_setting_length = 9;
	const char *menu_setting_en[menu_setting_length] = { "Train Name",
			"Train Route", "Current Station", "Display Language",
			"Menu Language", "Master / Slave Mode", "Manual / GPS Mode",
			"Coach List", "System Info" };
	String menu_setting_bl[menu_setting_length] = {

	};
	byte currentDisplayState = DISP_IDLE;
	Train_Detail tempTrainDetail;
	byte currentMenuPointer = 0;

	void pageMain();
	void pageMenu(byte posMenu);

};

#endif /* MENUDISPLAY_H_ */
