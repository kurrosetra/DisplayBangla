/*
 * storageBank.h
 *
 * To store and load data to EEPROM for ATMega family
 * Illustration:
 *
 * ------------------------------------------------------------------
 * |[BLOCK 0 COUNTER]               |[BLOCK 2 COUNTER]              |
 * |--------------------------------|-------------------------------|
 * |                                |[COUNTER-2][DATA-2]            |
 * |                                |[COUNTER-1][DATA-2]            |
 * |                                |[COUNTER][NEWEST_DATA] <<------|--<< newest block of data
 * |                                |[OLD_COUNTER][OLD_DATA]        |
 * |                                |[OLD_COUNTER+1][OLD_DATA+1]    |
 * |                                |[OLD_COUNTER+1][OLD_DATA+2]    |
 * |--------------------------------|-------------------------------|
 * |[BLOCK 1 COUNTER]               |[BLOCK 3 COUNTER]              |
 * |--------------------------------|-------------------------------|
 * |                                |                               |
 * |                                |                               |
 * ------------------------------------------------------------------
 *
 * Note:
 * - each block has its own data counter
 *
 *  Created on: Mar 28, 2018
 *      Author: miftakur
 */

#ifndef STORAGEBANK_H_
#define STORAGEBANK_H_

#include "Arduino.h"
#include <EEPROM.h>

#define DEBUG							1
#if DEBUG
# define ADV_DEBUG						0
#endif	//#if DEBUG


// there's none that make COUNTER_MAX-1's modulo = 0
#define COUNTER_MAX						252

#define ERR_SPACE_SIZE					0b1
#define ERR_COUNTER_EXCEEDED			0b10
#define ERR_COUNTER_MODULO				0b100

class StorageBank: public EEPROMClass
{
private:
	uint16_t START_ADDRESS;   	    	// start storage address in EEPROM
	byte STORED_DATA_SIZE;        		// fixed size of data to store in EEPROM + 1 Byte Counter
	byte BLOCK_NUM;  		       		// max number of blocks to create
	byte BLOCK_SIZE;  		      		// max number of STORED_DATA_SIZE in each block
	byte newestBlockID;					// current active block
	byte newestBlockCounter;			// newest block counter, always < COUNTER_MAX
	byte newestDataCounter;				// newest data counter in block, always < COUNTER_MAX
	uint16_t newestDataAddr;			// newest data address
	byte error;

	/**
	 * update:
	 * - active_block
	 * - newestBlockCounter
	 */
	void updateBlock();
	byte getBlockID(uint16_t addr);
//	/**
//	 *  - change block's counter
//	 *  - find newestDataCounter which is at the end of next block
//	 *  - change first data block's counter
//	 */
//	void setupNextBlock();
	/**
	 * get the newestDataCounter, used for {@link read} & {@link write} function
	 *
	 * @param blockNum block ID
	 * @param *dataAddr
	 * @return newestDataCounter in the {@link blockNum}
	 */
	uint16_t getNewestDataCounter(uint16_t blockNum, uint16_t *dataAddr);
	void clearEEPROM();

public:
	/**
	 * constructor
	 * @param blockNum number of blocks to create
	 * @param blockSize number of STORED_DATA_SIZE in each block
	 * @param fixedDataSize fixed size of data to store in EEPROM
	 * @param startAddress start storage address to occupy in EEPROM
	 */
	StorageBank(byte blockNum, byte blockSize, byte fixedDataSize, uint16_t startAddress = 0);

	bool init();
	void read(byte *data);
	void write(byte *data);
	void errorMessage(String *s);
};

#endif /* STORAGEBANK_H_ */

