/*
 * lncv_prog.h
 *
 *  Created on: 16.09.2017
 *      Author: mblank
 */

#ifndef LNCV_PROG_H_
#define LNCV_PROG_H_

#include <Arduino.h>
#include <LocoNet.h>
#include "EEPROM.h"
#include "config.h"

#if defined (__cplusplus)
extern "C" {
#endif

void readLNCVsFromEEPROM(void);
void notifySensor(uint16_t Address, uint8_t State);
void notifySwitchReport(uint16_t Address, uint8_t State, uint8_t Sensor);
void debugLnPacket(void);
int8_t notifyLNCVread(uint16_t ArtNr, uint16_t lncvAddress, uint16_t,
		uint16_t & lncvValue);
int8_t notifyLNCVprogrammingStart(uint16_t & ArtNr, uint16_t & ModuleAddress);
int8_t notifyLNCVwrite(uint16_t ArtNr, uint16_t lncvAddress,
		uint16_t lncvValue);
void notifyLNCVprogrammingStop(uint16_t ArtNr, uint16_t ModuleAddress);

#if defined (__cplusplus)
}
#endif

#endif /* LNCV_PROG_H_ */
