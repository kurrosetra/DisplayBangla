/*
 * trainDetal.h
 *
 *  Created on: Mar 27, 2018
 *      Author: miftakur
 */

#ifndef TRAINDETAL_H_
#define TRAINDETAL_H_

#include "Arduino.h"

struct Coach_List
{
	char *name;
	byte number;
};

struct Train_Info
{
	uint16_t trainID;	// train's number
	uint16_t currentTrainPosition;  // position of current train in SD card
	uint16_t lastTrainPosition;
	String trainFiles;	// filename of selected's train
	String trainName;	// train's name
	String trainNum;	// train's number in string
	String currentStationName;	// current station's name
	uint16_t currentStationPosition;  // position of current station in trainFiles
	uint16_t lastStationPosition;
	String routeStart;	// first station name
	String routeEnd;	// last station name
	bool trainRoute;	// train's route (down=0, up=1)
};

struct Train_Detail
{
	byte lang;		// menu language (0=en, 1=ba)
	Train_Info trainInfo;		// current train's info
	Coach_List currentCoach;	// current coach's info
	bool masterMode;	// master/slave mode (slave=0, master=1)
	bool gpsMode;		// gps/manual mode (manual=0, gps=1)
};



#endif /* TRAINDETAL_H_ */
