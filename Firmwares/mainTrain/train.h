/*
 * train.h
 *
 *  Created on: Jul 7, 2018
 *      Author: miftakur
 */

#ifndef TRAIN_H_
#define TRAIN_H_

#include "Arduino.h"

#define COACH_NAME_MAX_SIZE						4
#define COACH_NAME_ADDRESS						0

typedef enum
{
	SLAVE_MODE,
	MASTER_MODE
} Master_Mode_e;

typedef enum
{
	MANUAL_MODE,
	GPS_MODE
} Gps_Mode_e;

typedef enum
{
	STATION_ARRIVED = 0,
	STATION_TOWARD = 1,
	STATION_NEXT = 0,
	STATION_PREV = 1
} Station_State_e;

typedef struct
{
		uint16_t trainID;  								// train's number
		String trainName;  								// train's name
		/* MASTER ONLY */
		byte trainPosition;						// // position of current train's name in allTrainsName struct
} Train_Info_t;

typedef struct
{
		Station_State_e state;
		String name;
		uint16_t size;
		String first;
		String end;
		/* MASTER ONLY */
		uint16_t pos;
} Station_Info_t;

const byte FILES_MAX_SIZE = 20;
typedef struct
{
		byte size;
		uint16_t trainID[FILES_MAX_SIZE];
		String trainName[FILES_MAX_SIZE];
} Train_Filename_t;

typedef struct
{
		bool masterMode;
		bool gpsMode;
		String coachName;
		Train_Info_t trainInfo;
		Station_Info_t stationInfo;
} Train_Detail_t;

#endif /* TRAIN_H_ */
