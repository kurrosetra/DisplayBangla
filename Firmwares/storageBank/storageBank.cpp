/*
 * storageBank.cpp
 *
 *  Created on: Mar 28, 2018
 *      Author: miftakur
 */

#include "storageBank.h"

StorageBank::StorageBank(byte blockNum, byte blockDataSize, byte fixedDataSize,
		uint16_t startAddress)
{
	error = 0;
	START_ADDRESS = startAddress;
	BLOCK_NUM = blockNum;
	BLOCK_SIZE = blockDataSize;
	STORED_DATA_SIZE = fixedDataSize + 1;
	newestBlockID = 0;
	newestBlockCounter = 0;
	newestDataAddr = 0;
	newestDataCounter = 0;
}

bool StorageBank::init()
{
	uint16_t eeprom_size_requirement;
	uint16_t eeprom_size_available = E2END + 1;
	uint16_t dataBlockSize = 0;

	dataBlockSize = (uint16_t) BLOCK_NUM * (uint16_t) BLOCK_SIZE;
	eeprom_size_requirement = (dataBlockSize * (uint16_t) STORED_DATA_SIZE) + BLOCK_NUM /*BLOCK HEADER*/
	+ START_ADDRESS;

	if (eeprom_size_requirement >= eeprom_size_available)
		error |= ERR_SPACE_SIZE;

	if (BLOCK_SIZE >= COUNTER_MAX)
		error |= ERR_COUNTER_EXCEEDED;

	if (BLOCK_NUM * BLOCK_SIZE >= COUNTER_MAX)
		error |= ERR_COUNTER_MODULO;

	//unformatted eeprom space
	if (EEPROM.read(START_ADDRESS) == 0xFF)
		clearEEPROM();

	//find active block
	updateBlock();
	//find data counter at active block
	newestDataCounter = getNewestDataCounter(newestBlockID, &newestDataAddr);

	return error;
}

void StorageBank::read(byte* data)
{
	byte a;

	//read data content
	for ( a = 1; a < STORED_DATA_SIZE; a++ )
		*data++ = EEPROM.read(newestDataAddr + a);
}

void StorageBank::write(byte* data)
{
	byte a;
	uint16_t blockAddrStart, blockAddrEnd;

	//if already at the end of block, setup next block
	blockAddrStart = START_ADDRESS + 1 + (newestBlockID * BLOCK_SIZE * STORED_DATA_SIZE)
			+ newestBlockID;
	blockAddrEnd = blockAddrStart + (BLOCK_SIZE * STORED_DATA_SIZE) - STORED_DATA_SIZE;

#if DEBUG
	Serial.print(F("blockAddrStart= "));
	Serial.println(blockAddrStart);
	Serial.print(F("blockAddrEnd= "));
	Serial.println(blockAddrEnd);
#endif	//#if DEBUG

	if (newestDataAddr >= blockAddrEnd) {
		//- increment next block id & counter
		newestBlockID++;
		if (newestBlockID >= BLOCK_NUM)
			newestBlockID = 0;

		newestBlockCounter++;
		if (newestBlockCounter > COUNTER_MAX)
			newestBlockCounter = 0;

		if (newestBlockID == 0)
			blockAddrStart = START_ADDRESS + 1;
		else
			blockAddrStart = blockAddrEnd + STORED_DATA_SIZE + 1;
		blockAddrEnd = blockAddrStart + (BLOCK_SIZE * STORED_DATA_SIZE) - STORED_DATA_SIZE;

#if DEBUG
		Serial.print(F("New blockAddrStart= "));
		Serial.println(blockAddrStart);
		Serial.print(F("New blockAddrEnd= "));
		Serial.println(blockAddrEnd);
#endif	//#if DEBUG

		EEPROM.write(blockAddrStart - 1, newestBlockCounter);
		//- find its newestDataCounter, which is at the end of block, and increment it
		newestDataCounter = EEPROM.read(blockAddrEnd);

		//unwritten
		if (newestDataCounter > COUNTER_MAX)
			newestDataCounter = 0;
		else {
			newestDataCounter++;
			if (newestDataCounter > COUNTER_MAX)
				newestDataCounter = 0;
		}

		//- put it at the start of block
		newestDataAddr = blockAddrStart;
#if DEBUG
		Serial.print(F("prev newestDataCounter= "));
		Serial.print(EEPROM.read(blockAddrEnd));
		Serial.print(F(" @"));
		Serial.println(blockAddrEnd);

		Serial.print(F("newestDataCounter= "));
		Serial.print(newestDataCounter);
		Serial.print(F(" @"));
		Serial.println(newestDataAddr);

#endif	//#if DEBUG
	}	//(newestDataAddr == blockAddrEnd)
	else {
		if (newestDataCounter > COUNTER_MAX)
			newestDataCounter = 0;
		else {
			newestDataCounter++;
			newestDataAddr += STORED_DATA_SIZE;
		}
	}

	EEPROM.write(newestDataAddr, newestDataCounter);
	//- update data
	for ( a = 1; a < STORED_DATA_SIZE; a++ )
		EEPROM.write(newestDataAddr + a, *data++);
}

void StorageBank::updateBlock()
{
	byte prevBlockCounter = 0;
	byte blockCounter = 0;
	uint16_t prevBlockCounterAddr = 0;
	uint16_t blockCounterAddr = 0;
	uint16_t temAddr = 0;
	byte a;
	bool validBlockFormat = 0;

	blockCounterAddr = START_ADDRESS;
	blockCounter = EEPROM.read(blockCounterAddr);
	prevBlockCounterAddr = START_ADDRESS + ((BLOCK_NUM - 1) * BLOCK_SIZE * STORED_DATA_SIZE)
			+ (BLOCK_NUM - 1);
	prevBlockCounter = EEPROM.read(prevBlockCounterAddr);

	if (prevBlockCounter >= COUNTER_MAX) {
		if (blockCounter == 0 || blockCounter == 0xFF)
			validBlockFormat = 1;
	}
	else {
		if ((blockCounter == prevBlockCounter + 1)
				|| (prevBlockCounter == blockCounter + BLOCK_NUM - 1))
			validBlockFormat = 1;
	}

#if DEBUG
	Serial.println();
	Serial.print(F("prevBlockCounterAddr= "));
	Serial.println(prevBlockCounterAddr);
	Serial.print(F("prevBlockCounter= "));
	Serial.println(prevBlockCounter);
	Serial.print(F("blockCounter= "));
	Serial.println(blockCounter);
	Serial.print(F("validBlockFormat= "));
	Serial.println(validBlockFormat);
	Serial.println();
#endif	//#if DEBUG

	if (!validBlockFormat) {
		clearEEPROM();
		prevBlockCounter = 0;
	}
	else {
		prevBlockCounterAddr = blockCounterAddr;
		prevBlockCounter = blockCounter;

		//update block counter & its address
		temAddr = START_ADDRESS;
		for ( a = 1; a < BLOCK_NUM; a++ ) {
			temAddr += (BLOCK_SIZE * STORED_DATA_SIZE) + 1;

			blockCounterAddr = temAddr;
			blockCounter = EEPROM.read(blockCounterAddr);

#if DEBUG
			Serial.print(F("prev:now = "));
			Serial.print(prevBlockCounter);
			Serial.print(':');
			Serial.println(blockCounter);
#endif	//#if DEBUG

			if (blockCounter == prevBlockCounter + 1) {
				prevBlockCounterAddr = blockCounterAddr;
				prevBlockCounter = blockCounter;
			}
			else
				break;
		}

		//update block ID
		newestBlockID = getBlockID(prevBlockCounterAddr);

		newestBlockCounter = prevBlockCounter;
		//update data counter & its address
		newestDataCounter = getNewestDataCounter(newestBlockID, &newestDataAddr);
	}

#if DEBUG
	Serial.println();
	Serial.print(F("newestBlockID= "));
	Serial.println(newestBlockID);
	Serial.print(F("block counter= "));
	Serial.println(newestBlockCounter);
	Serial.println();
	Serial.print(F("newestDataCounter= "));
	Serial.println(newestDataCounter);
	Serial.print(F("newestDataAddr= "));
	Serial.println(newestDataAddr);
	Serial.println();
#endif	//#if DEBUG

}

byte StorageBank::getBlockID(uint16_t addr)
{
	byte id = 0;
	uint16_t blockStart, blockEnd;
	byte a;

	for ( a = 0; a < BLOCK_NUM; a++ ) {
		blockStart = START_ADDRESS + (a * BLOCK_SIZE * STORED_DATA_SIZE) + a;
		blockEnd = START_ADDRESS + ((a + 1) * BLOCK_SIZE * STORED_DATA_SIZE) + (a + 1);
#if DEBUG
		Serial.print(F("start:end -> "));
		Serial.print(blockStart);
		Serial.print(':');
		Serial.println(blockEnd);
#endif	//#if DEBUG

		if (addr >= blockStart && addr < blockEnd) {
			id = a;
			break;
		}
	}

	return id;
}

uint16_t StorageBank::getNewestDataCounter(uint16_t blockNum, uint16_t* dataAddr)
{
	byte prevDataCounter = 0;
	uint16_t prevDataAddress = 0;
	uint16_t dataAddress = 0;
	uint16_t blockAddress = 0;
	uint16_t startBlockAddress = 0;
	byte dataCounter;
	byte a, b;
	bool validBlockFormat = 0;

	// get block's start data address
	blockAddress = START_ADDRESS + (blockNum * BLOCK_SIZE * STORED_DATA_SIZE) + blockNum;
	dataAddress = blockAddress + 1;
	// get block's end data address
	blockAddress = START_ADDRESS + ((blockNum + 1) * BLOCK_SIZE * STORED_DATA_SIZE) + blockNum + 1;
	prevDataAddress = blockAddress - STORED_DATA_SIZE;

	//get start data counter & end data counter
	dataCounter = EEPROM.read(dataAddress);
	prevDataCounter = EEPROM.read(prevDataAddress);

	if (prevDataCounter >= COUNTER_MAX) {
		if (dataCounter == 0 || dataCounter > COUNTER_MAX)
			validBlockFormat = 1;
	}
	else {
		if ((dataCounter == prevDataCounter + 1)
				|| (prevDataCounter = dataCounter + BLOCK_SIZE - 1))
			validBlockFormat = 1;
	}

#if DEBUG
	Serial.println();
	Serial.print(F("block num= "));
	Serial.println(blockNum);

	Serial.print(F("prevDataCounter= "));
	Serial.print(prevDataCounter, HEX);
	Serial.print(F(" @ "));
	Serial.println(prevDataAddress);
	Serial.print(F("dataCounter= "));
	Serial.print(dataCounter, HEX);
	Serial.print(F(" @ "));
	Serial.println(dataAddress);
	Serial.println();
#endif	//#if DEBUG

	if (!validBlockFormat) {
#if DEBUG
		Serial.print(F("data format incorrect, start clearing blockNum-"));
		Serial.println(blockNum);
#endif	//#if DEBUG
		*dataAddr = dataAddress;
		for ( a = 0; a < BLOCK_SIZE; a++ ) {
			for ( b = 0; b < STORED_DATA_SIZE; b++ )
				EEPROM.write(dataAddress++, 0xFF);
		}
		prevDataCounter = COUNTER_MAX;
#if DEBUG
		Serial.println(F("done clearing block"));
#endif	//#if DEBUG

	}
	else {
		prevDataAddress = dataAddress;
		prevDataCounter = dataCounter;

#if DEBUG
		Serial.print(F("start data address: "));
		Serial.println(dataAddress);
		Serial.println(BLOCK_SIZE);
		Serial.println(STORED_DATA_SIZE);
#endif	//#if DEBUG

		startBlockAddress= dataAddress;
		if (prevDataCounter != COUNTER_MAX) {
			for ( a = 1; a < BLOCK_SIZE; a++ ) {
				dataAddress = startBlockAddress + (a * STORED_DATA_SIZE);
				dataCounter = EEPROM.read(dataAddress);

#if DEBUG
				Serial.println(a);
				Serial.print(F("prevDataCounter= "));
				Serial.print(prevDataCounter, HEX);
				Serial.print(F(" @"));
				Serial.println(prevDataAddress);
				Serial.print(F("dataCounter= "));
				Serial.print(dataCounter, HEX);
				Serial.print(F(" @"));
				Serial.println(dataAddress);
#endif	//#if DEBUG

				if (dataCounter == prevDataCounter + 1) {
					prevDataCounter = dataCounter;
					prevDataAddress = dataAddress;
				}
				else
					//found newest
					break;
			}
		}

		*dataAddr = prevDataAddress;
	}

#if DEBUG
	Serial.print(prevDataCounter, HEX);
	Serial.print(F(" @"));
	Serial.println(prevDataAddress);
#endif	//#if DEBUG

	return prevDataCounter;
}

void StorageBank::clearEEPROM()
{
	uint16_t spaceEnded = START_ADDRESS + (BLOCK_NUM * BLOCK_SIZE * STORED_DATA_SIZE) + BLOCK_NUM;

#if DEBUG
	Serial.println(F("Clearing space:"));
	Serial.print(F("address start= "));
	Serial.println(START_ADDRESS);
	Serial.print(F("address end= "));
	Serial.println(spaceEnded);
#endif	//#if DEBUG

	EEPROM.write(START_ADDRESS, 0);
	for ( uint16_t a = START_ADDRESS + 1; a < spaceEnded + 1; a++ ) {
		if (EEPROM.read(a) != 0xFF)
			EEPROM.write(a, 0xFF);
	}

	newestBlockID = 0;
	newestBlockCounter = 0;
	newestDataAddr = START_ADDRESS + 1;
	newestDataCounter = 0xFF;

#if DEBUG
	Serial.println(F("done clearing!"));
#endif	//#if DEBUG

}

void StorageBank::errorMessage(String* s)
{
	*s = "";

	if (error & ERR_SPACE_SIZE)
		*s += F("Space size exceeded current micro\r\n");
	if (error & ERR_COUNTER_EXCEEDED)
		*s += F("block size >= COUNTER_MAX (251)\r\n");
	if (error & ERR_COUNTER_MODULO)
		*s += F("blockNum * blockSize != COUNTER_MAX (251)\r\n");
}
