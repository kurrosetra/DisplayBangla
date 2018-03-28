/*
 * storageBank.cpp
 *
 *  Created on: Mar 28, 2018
 *      Author: miftakur
 */

#include "storageBank.h"

StorageBank::StorageBank(byte blockNum, byte blockDataSize, byte fixedDataSize, uint16_t startAddress, byte counterMax)
{
	START_ADDRESS = startAddress;
	BLOCK_NUM_SIZE = blockNum;
	BLOCK_DATA_SIZE = blockDataSize;
	STORED_DATA_SIZE = fixedDataSize + 1;
	block_address_pointer = 0;
	data_address_pointer = 0;
	active_block = 0;
	sizeError = 0;
	counter_pointer = 0;
	counter_max = counterMax;
	if (counter_max > BLOCK_COUNTER_MAX)
		counter_max = BLOCK_COUNTER_MAX;
}

bool StorageBank::init()
{
	bool ret = 1;
	uint16_t eeprom_size_available = E2END;
	uint16_t eeprom_size_requirement;

	eeprom_size_requirement = (BLOCK_NUM_SIZE * BLOCK_DATA_SIZE * STORED_DATA_SIZE) + 1 /*BLOCK HEADER*/+ START_ADDRESS;

	if ((eeprom_size_requirement >= eeprom_size_available) || (BLOCK_DATA_SIZE >= BLOCK_COUNTER_MAX)) {
		sizeError = 1;
		return 0;
	}

	active_block = EEPROM.read(START_ADDRESS);
	if (active_block == 0xFF) {
		active_block = 0;
		EEPROM.write(START_ADDRESS, 0);

		counter_pointer = 0xFF;
		data_address_pointer = START_ADDRESS + 1;
		firstWrite = 1;
	}
	else
		//find newest data address
		counter_pointer = findCurrentDataAddress();

#if DEBUG
	Serial.print(F("active_block= "));
	Serial.println(active_block);
	Serial.print(F("data_address_pointer= "));
	Serial.println(data_address_pointer);
	Serial.print(F("counter_pointer= "));
	Serial.println(counter_pointer);
	Serial.print(F("counter_max= "));
	Serial.println(counter_max);
#endif	//#if DEBUG

	return ret;
}

void StorageBank::read(byte* data)
{
	for ( byte a = 1; a < STORED_DATA_SIZE; a++ )
		*(data++) = EEPROM.read(data_address_pointer + a);
}

void StorageBank::write(byte* data)
{
	uint16_t nextDataAddressPointer = 0;
	uint16_t currentBlockStartAddress;
	uint16_t currentBlockEndAddress = 0;
	byte a;

	nextDataAddressPointer = data_address_pointer + STORED_DATA_SIZE;
	currentBlockStartAddress = (active_block * BLOCK_DATA_SIZE) + START_ADDRESS + 1;
	currentBlockEndAddress = currentBlockStartAddress + ((BLOCK_DATA_SIZE - 1) * STORED_DATA_SIZE);

	//next data address is outside active_block boundary
	if (nextDataAddressPointer > currentBlockEndAddress) {
		//already at end of block
		if (active_block >= BLOCK_NUM_SIZE - 1) {
			active_block = 0;
			data_address_pointer = START_ADDRESS + 1;
		}
		else {
			active_block++;
			data_address_pointer = nextDataAddressPointer;
		}

		//find counter_pointer for this block, which is at the end of data block
		currentBlockStartAddress = (active_block * BLOCK_DATA_SIZE) + START_ADDRESS + 1;
		currentBlockEndAddress = currentBlockStartAddress + ((BLOCK_DATA_SIZE - 1) * STORED_DATA_SIZE);
		counter_pointer = EEPROM.read(currentBlockEndAddress) + 1;
		if (counter_pointer > counter_max)
			counter_pointer = 0;

	}  // nextDataAddressPointer >= currentBlockEndAddress
	else {
		counter_pointer++;
		if (counter_pointer > counter_max)
			counter_pointer = 0;

		//if not unwritten space
		if (EEPROM.read(data_address_pointer) != 0xFF && !firstWrite)
			data_address_pointer = nextDataAddressPointer;

		if (firstWrite)
			firstWrite = 0;

	}	// else nextDataAddressPointer >= currentBlockEndAddress

#if DEBUG
	byte b;
	Serial.print(F("data_address_pointer= "));
	Serial.println(data_address_pointer);
	Serial.print(F("counter_pointer= "));
	Serial.println(counter_pointer);
	//update counter
	EEPROM.write(data_address_pointer, counter_pointer);
	//update data
	for ( a = 1; a < STORED_DATA_SIZE; a++ ) {
		b = *data++;
		EEPROM.write(data_address_pointer + a, b);
		Serial.print(b);
		Serial.print(' ');
	}
	Serial.println();
#else
	//update counter
	EEPROM.write(data_address_pointer, counter_pointer);
	//update data
	for ( a = 1; a < STORED_DATA_SIZE; a++ )
		EEPROM.write(data_address_pointer + a, *data++);
#endif	//#if DEBUG

}

byte StorageBank::findCurrentDataAddress()
{
	uint16_t addr = 0;
	uint16_t temAddr = 0;
	uint16_t maxCounter = 0;
	uint16_t a, b;
	uint16_t currentBlockStartAddress;

	currentBlockStartAddress = (active_block * BLOCK_DATA_SIZE) + START_ADDRESS + 1;

	maxCounter = EEPROM.read(currentBlockStartAddress);
	addr = currentBlockStartAddress;

	for ( a = 1; a < BLOCK_DATA_SIZE; a++ ) {
		temAddr = currentBlockStartAddress + (a * STORED_DATA_SIZE);
		b = EEPROM.read(temAddr);

		//unwritten block_data
		if (b == 0xFF)
			break;
		else {
			//counter reset found
			if (b == 0) {
				if (maxCounter == counter_max) {
					maxCounter = 0;
					addr = temAddr;
				}
				else
					break;
			}
			//if next counter is jump high
			else if (b > maxCounter + 1)
				break;
			else {
				maxCounter = b;
				addr = temAddr;
			}
		}	// else b == 0xFF
	}

	data_address_pointer = addr;

	return maxCounter;
}
