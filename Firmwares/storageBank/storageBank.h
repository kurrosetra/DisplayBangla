/*
 * storageBank.h
 *
 * To store and load data to EEPROM for ATMega family
 * Illustration:
 *
 * [Block 2] <<--------------------------------------------------------<< ACTIVE BLOCK at START_ADDRESS
 * ------------------------------------------------------------------
 * |[BLOCK 0]                       |[BLOCK 2]                      |
 * |--------------------------------|-------------------------------|
 * |                                |[COUNTER-2][DATA-2]            |
 * |                                |[COUNTER-1][DATA-2]            |
 * |                                |[COUNTER][NEWEST_DATA] <<------|--<< newest block of data
 * |                                |[OLD_COUNTER][OLD_DATA]        |
 * |                                |[OLD_COUNTER+1][OLD_DATA+1]    |
 * |                                |[OLD_COUNTER+1][OLD_DATA+2]    |
 * |--------------------------------|-------------------------------|
 * |[BLOCK 1]                       |[BLOCK 3]                      |
 * |--------------------------------|-------------------------------|
 * |                                |                               |
 * |                                |                               |
 * ------------------------------------------------------------------
 *
 *  Created on: Mar 28, 2018
 *      Author: miftakur
 */

#ifndef STORAGEBANK_H_
#define STORAGEBANK_H_

#include "Arduino.h"
#include <EEPROM.h>

#define DEBUG							0

//BLOCK_COUNTER_MAX != 0xFF (default value unwritten eeprom)
#define BLOCK_COUNTER_MAX				254
//BLOCK_DATA_COUNTER_MAX < BLOCK_COUNTER_MAX, to avoid duplicate block counter in a block
#define BLOCK_DATA_COUNTER_MAX			253

class StorageBank: public EEPROMClass
{
private:
	uint16_t START_ADDRESS;   	    	// start storage address in EEPROM
	byte BLOCK_NUM_SIZE;         		// max number of blocks to create
	byte BLOCK_DATA_SIZE;        		// max number of block STORED_DATA_SIZE in each block
	byte STORED_DATA_SIZE;        		// fixed size of data to store in EEPROM + 1 Byte Counter
	byte active_block;					// current active block
	uint16_t block_address_pointer;		// address pointer to active block
	uint16_t data_address_pointer;		// address pointer to newest data
	byte counter_pointer;				// current counter in active block
	byte counter_max;					// max of counter_pointer
	bool sizeError;
	bool firstWrite=0;

	byte findCurrentDataAddress();

public:
	/**
	 * constructor
	 * @param blockNum number of blocks to create
	 * @param blockDataSize number of block STORED_DATA_SIZE in each block
	 * @param fixedDataSize fixed size of data to store in EEPROM
	 * @param startAddress start storage address to occupy in EEPROM
	 */
	StorageBank(byte blockNum, byte blockDataSize, byte fixedDataSize, uint16_t startAddress = 0, byte counterMax =
			BLOCK_DATA_COUNTER_MAX);

	bool init();
	void read(byte *data);
	void write(byte *data);
};

#endif /* STORAGEBANK_H_ */

