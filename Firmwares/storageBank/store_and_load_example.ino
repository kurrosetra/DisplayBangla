#include "Arduino.h"
#include "storageBank.h"

StorageBank sb(3, 10, 4, 0, 25);

void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Example usage of storageBank.h"));
	Serial.println(F("=============================="));
	Serial.println();

	Serial.print(F("size of EEPROM= "));
	Serial.println(E2END + 1);

	EEPROM.write(0, 0xFF);
	sb.init();
}

void loop()
{
	char c;

	if (Serial.available()) {
		c = Serial.read();
		switch (c)
		{
		case '1':
			Serial.println(F("Start test1"));
			test1();
			break;
		case 't':
			Serial.println(F("Start writing"));
			test();
			break;
		case 'r':
			Serial.println(F("Start reading"));
			read();
			break;
		case '0':
			Serial.println(F("reset"));
			EEPROM.write(0, 0xFF);
			sb.init();
			break;
		}
	}

}

void test1()
{
	byte a;
	static byte firstByte = 0;
	byte data1[4] = { firstByte, 1, 2, 3 };
	byte data2[4] = { firstByte + 4, 5, 6, 7 };
	firstByte++;

	Serial.println(F("before"));
	Serial.print(F("active_block= "));
	Serial.println(EEPROM.read(0));
	for ( a = 1; a < 21; a++ ) {
		Serial.print(EEPROM.read(a));
		Serial.print(' ');
	}
	Serial.println();

	sb.write(data1);

	Serial.println(F("write 1"));
	Serial.print(F("active_block= "));
	Serial.println(EEPROM.read(0));
	for ( a = 1; a < 21; a++ ) {
		Serial.print(EEPROM.read(a));
		Serial.print(' ');
	}
	Serial.println();

	sb.write(data2);

	Serial.println(F("write 2"));
	Serial.print(F("active_block= "));
	Serial.println(EEPROM.read(0));
	for ( a = 1; a < 21; a++ ) {
		Serial.print(EEPROM.read(a));
		Serial.print(' ');
	}
	Serial.println();

}

void test()
{
	byte a, b;
	//data= [block]00[counter]
	byte data[4];
	static byte counter = 0;
	bool brk = 0;

	Serial.println();
	for ( a = 0; a < 5; a++ ) {
		for ( b = 0; b < 10; b++ ) {
			if ((a >= 4) && (b > 1)) {
				brk = 1;
				b--;
				break;
			}
			else {
				data[0] = a;
				data[1] = 0;
				data[2] = 0;
				data[3] = counter++;
				if (counter >= 200)
					counter = 0;

				sb.write(data);
				Serial.print(". ");
			}
		}

		if (brk)
			break;
		Serial.println();
	}
	Serial.println();

	Serial.println(F("newest data at:"));
	Serial.print(F("Block: "));
	Serial.println(a);
	Serial.print(F("line: "));
	Serial.println(b);
	Serial.print(F("Data write: "));
	Serial.print(data[0]);
	Serial.print(' ');
	Serial.print(data[1]);
	Serial.print(' ');
	Serial.print(data[2]);
	Serial.print(' ');
	Serial.print(data[3]);
	Serial.print(' ');
	Serial.println();

	//
	Serial.println();
	for ( a = 1; a < 21; a++ ) {
		Serial.print(EEPROM.read(a));
		Serial.print(' ');
	}
	Serial.println();
	for ( a = 51; a < 66; a++ ) {
		Serial.print(EEPROM.read(a));
		Serial.print(' ');
	}
	Serial.println();

}

void read()
{
	byte data[4];

	sb.read(data);
	Serial.print(F("Data read: "));
	Serial.print(data[0]);
	Serial.print(' ');
	Serial.print(data[1]);
	Serial.print(' ');
	Serial.print(data[2]);
	Serial.print(' ');
	Serial.print(data[3]);
	Serial.print(' ');
	Serial.println();
}
