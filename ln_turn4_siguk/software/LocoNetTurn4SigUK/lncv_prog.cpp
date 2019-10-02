/*
 * lncv_prog.c
 *****************************************************************************
 *
 *  Copyright (C) 2013 Damian Philipp
 *  Copyright (C) 2017 Michael Blank (store in EEPROM)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************
 *
 * DESCRIPTION
 *
 * Most parts of the code are copied from the LocoNetLNCVDemo example
 * (part of the LocoNet library)
 *
 * The LNCVs are stored in EEPROM, thus all settings are still
 * available after a reset of the Arduino.
 *
 ******************************************************************************/

#include "lncv_prog.h"
#include <HardwareSerial.h>

extern HardwareSerial Serial;
extern lnMsg *LnPacket;
extern uint16_t debug;

extern void finishProgramming(void);

uint16_t lncv[LNCV_COUNT];   // values of LNCVs
LocoNetCVClass lnCV;
boolean programmingModeEnabled = false;
boolean programmingMode = false;

void readLNCVsFromEEPROM() {
	EEPROM.get(0, lncv);   // read inital lncv values
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Sensor messages
void notifySensor(uint16_t Address, uint8_t State) {
	Serial.print("notify Sensor: ");
	Serial.print(Address, DEC);
	Serial.print(" - ");
	Serial.println(State ? "Active" : "Inactive");
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Sensor Report messages
void notifySwitchReport(uint16_t Address, uint8_t State, uint8_t Sensor) {
	Serial.print("Switch Sensor Report: ");
	Serial.print(Address, DEC);
	Serial.print(':');
	Serial.print(Sensor ? "Switch" : "Aux");
	Serial.print(" - ");
	Serial.println(State ? "Active" : "Inactive");
}

// print current LNPacket to serial port
void debugLnPacket() {
	uint8_t opcode = (int) LnPacket->sz.command;
	// First print out the packet in HEX
	Serial.print("RX: ");
	uint8_t msgLen = getLnMsgSize(LnPacket);
	for (uint8_t x = 0; x < msgLen; x++) {
		uint8_t val = LnPacket->data[x];
		// Print a leading 0 if less than 16 to make 2 HEX digits
		if (val < 16)
			Serial.print('0');
		Serial.print(val, HEX);
		Serial.print(' ');
	}
	Serial.println();
}

/**
 * Notifies the code on the reception of a read request.
 * Note that without application knowledge (i.e., art.-nr., module address
 * and "Programming Mode" state), it is not possible to distinguish
 * a read request from a programming start request message.
 */
int8_t notifyLNCVread(uint16_t ArtNr, uint16_t lncvAddress, uint16_t,
		uint16_t & lncvValue) {
	Serial.print(F("Enter notifyLNCVread("));
	Serial.print(ArtNr, HEX);
	Serial.print(", ");
	Serial.print(lncvAddress, HEX);
	Serial.print(", ");
	Serial.print(", ");
	Serial.print(lncvValue, HEX);
	Serial.print(")");
// Step 1: Can this be addressed to me?
// All ReadRequests contain the ARTNR. For starting programming, we do not accept the broadcast address.
	if (programmingMode) {
		if (ArtNr == ARTNR) {
			if (lncvAddress < LNCV_COUNT) {
				lncvValue = lncv[lncvAddress];
				Serial.print(" LNCV Value: ");
				Serial.print(lncvValue);
				Serial.print("\n");
				return LNCV_LACK_OK;
			} else {
				// Invalid LNCV address, request a NAXK
				return LNCV_LACK_ERROR_UNSUPPORTED;
			}
		} else {
			Serial.print("ArtNr invalid.\n");
			return -1;
		}
	} else {
		Serial.print("Ignoring Request.\n");
		return -1;
	}
}

int8_t notifyLNCVprogrammingStart(uint16_t & ArtNr, uint16_t & ModuleAddress) {
// Enter programming mode. If we already are in programming mode,
// we simply send a response and nothing else happens.
	Serial.print(F("notifyLNCVProgrammingStart "));
	if (ArtNr == ARTNR) {
		Serial.print("artnrOK ");
		if (ModuleAddress == lncv[0]) {
			Serial.print(F("SINGLE module ENTERING PROGRAMMING MODE\n"));
			programmingMode = true;
			return LNCV_LACK_OK;
		} else if (ModuleAddress == 0xFFFF) {
			Serial.print(F("moduleBroadcast ENTERING PROGRAMMING MODE\n"));
			ModuleAddress = lncv[0];
			return LNCV_LACK_OK;
		}
	} else {
		if (debug) {
			Serial.print("ArtNr=");
			Serial.print(ArtNr);
			Serial.print("Mod.Addr");
			Serial.println(ModuleAddress);
		}
	}
	Serial.print(F("Ignoring Request.\n"));
	return -1;
}

/**
 * Notifies the code on the reception of a write request
 */
int8_t notifyLNCVwrite(uint16_t ArtNr, uint16_t lncvAddress,
		uint16_t lncvValue) {

	if (debug)
		Serial.print(F("notifyLNCVwrite, "));
//  dumpPacket(ub);
	if (!programmingMode) {
		Serial.print(F("not in Programming Mode.\n"));
		return -1;
	}

	if (ArtNr == ARTNR) {
		if (debug)
			Serial.print(" Artnr OK, ");

		if (lncvAddress < LNCV_COUNT) {
			lncv[lncvAddress] = lncvValue;
			if (debug) {
				Serial.print(F("write LNCV["));
				Serial.print(lncvAddress);
				Serial.print("]=");
				Serial.println(lncv[lncvAddress]);
			}
			EEPROM.put(0, lncv);
			return LNCV_LACK_OK;
		} else {
			Serial.print("lncvAddress Invalid.\n");
			return LNCV_LACK_ERROR_UNSUPPORTED;
		}

	} else {
		Serial.print("Artnr Invalid.\n");
		return -1;
	}
}

/**
 * Notifies the code on the reception of a request to end programming mode
 */
void notifyLNCVprogrammingStop(uint16_t ArtNr, uint16_t ModuleAddress) {
	Serial.print(F("notifyLNCVprogrammingStop "));
	Serial.print(" ArtNr=");
	Serial.print(ArtNr);
	Serial.print(" M-Adr=");
	Serial.println(ModuleAddress);
	if (programmingMode) {
		if (ArtNr == ARTNR /* && ModuleAddress == lncv[0] */) {
			// Uhlenbrock SW sends in some cases the wrong
			// module address when prog.-stop, therefor m-addr ignored
			finishProgramming();
		} else {
			if (ArtNr != ARTNR) {
				Serial.print("Wrong Artnr.\n");
				return;
			}
			/*if (ModuleAddress != lncv[0]) {
			 Serial.print("Wrong Module Address.\n");
			 return;
			 } */
		}
	} else {
		Serial.print("Ignoring Request.\n");
	}
}
