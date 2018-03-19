/*
 * menuDisplay.h
 *
 *  Created on: Mar 19, 2018
 *      Author: miftakur
 */

#ifndef MENUDISPLAY_H_
#define MENUDISPLAY_H_

#include <U8g2lib.h>
#include <SPI.h>

class menuDisplay
{
private:
	U8G2_ST7920_128X64_F_HW_SPI *lcd;

	byte _lcd_backligth_pin;
	static const byte menu_setting_length = 8;
	String menu_setting_en[menu_setting_length] = { "Train Name", "Train Route",
			"Current Station", "Display Language", "Menu Language",
			"Master / Slave Mode", "Manual / GPS Mode", "System Info" };
	String menu_setting_bl[menu_setting_length] = {

	};
//	String menu_setting_id[menu_setting_length] = { "Nama Kereta",
//			"Jalur Kereta", "Stasiun Saat Init", "Bahasa Tampilan",
//			"Bahasa Pilihan", "Mode Master/Slave", "Mode Manual/GPS",
//			"Informasi Sistem" };

public:
	menuDisplay(byte ss, byte backlight)
	{
		lcd = new U8G2_ST7920_128X64_F_HW_SPI(U8G2_R0, ss);
		_lcd_backligth_pin = backlight;
	}

	void init();
	void backlight(bool onf);

	void pageMain(byte master, byte gps = 0);
	void pageSubMenu(String title, String opt1, String opt2, byte selection);
	void pageSubMenu(String title, String opt1, String opt2, String opt3,
			byte selection);

};

#endif /* MENUDISPLAY_H_ */
