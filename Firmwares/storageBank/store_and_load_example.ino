#include "Arduino.h"
#include "storageBank.h"

const byte blockNum = 3;
const byte blockSize = 5;
const byte dataSize = 2;
const uint16_t startAddress = 3000;

StorageBank sb(blockNum, blockSize, dataSize, startAddress);

void setup()
{
	String errMessage;
	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Example usage of storageBank.h"));
	Serial.println(F("=============================="));
	Serial.println();

	Serial.print(F("size of EEPROM= "));
	Serial.println(E2END + 1);

	if (sb.init()) {
		sb.errorMessage(&errMessage);
		Serial.println(errMessage);
	}
}

void viewStorageBank()
{
	byte block;
	byte dataBlock;
	byte data;
	uint16_t addr = startAddress;

	Serial.print(F("Block number= "));
	Serial.println(blockNum);
	Serial.print(F("block size= "));
	Serial.println(blockSize);
	Serial.print(F("data size= "));
	Serial.println(dataSize);

	for ( block = 0; block < blockNum; block++ ) {
		Serial.println(F("==========="));
		Serial.print(F("addr= "));
		Serial.println(addr);
		Serial.print(F("Block["));
		Serial.print(block);
		Serial.print(F("] Counter= "));
		Serial.println(EEPROM.read(addr));
		Serial.println(F("==========="));
		addr++;
		for ( dataBlock = 0; dataBlock < blockSize; dataBlock++ ) {
			for ( data = 0; data < dataSize + 1; data++ ) {
				Serial.print(EEPROM.read(addr++), HEX);
				Serial.print(' ');
			}
			Serial.println();
		}
		Serial.println();
	}

}

void test0()
{
	static byte counter = 0;
	byte data[dataSize];

	memset(data, 0, dataSize);
	data[0] = counter++;

	sb.write(data);
}

void test1()
{
	uint16_t dataAddress[5] = { 3017, 3020, 3023, 3026, 3029 };
	byte val = COUNTER_MAX - 4;

	for ( int a = 0; a < 5; a++ ) {
		EEPROM.write(dataAddress[a], val++);
	}
}

void loop()
{
	char c;
	byte dataRead[dataSize];
	memset(dataRead, 0, dataSize);

	if (Serial.available()) {
		c = Serial.read();
		switch (c)
		{
		case '1':
			Serial.println(F("Test 0"));
			test0();
			break;
		case '2':
			Serial.println(F("Test 1"));
			test1();
			break;
		case 'r':
			Serial.println(F("read:"));
			sb.read(dataRead);
			for ( int i = 0; i < dataSize; i++ ) {
				Serial.print(dataRead[i], HEX);
				Serial.print(' ');
			}
			Serial.println();
			break;
		case 'c':
			EEPROM.write(startAddress, 0xFF);
			sb.init();
			break;
		case 'v':
			Serial.println(F("view storage bank:"));
			viewStorageBank();
			break;
		}
	}

}

