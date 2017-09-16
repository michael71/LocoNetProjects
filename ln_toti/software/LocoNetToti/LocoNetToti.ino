/*******************************************************************************************
 * LocoNetToti 
 * (based on: SX_Belegtmelder_V0210)
 *
 * 
 * Copyright (C) 2017 Michael Blank
 * Belegtmelder: Copyright (C) 2015 Reinhard Thamm
 * LNCV part:    Copyright (C) 2013 Damian Philipp
 *
 * Zielhardware/-controller: Arduino Pro Mini (5V, 16MHz), ATmega328
 * Basis LN 1.0 + BM-Aufsteckplatine R. Thamm
 * 
 * Adresse des Moduls und Abfallverzögerung werden im EEPROM des Controllers abgespeichert.
 * 
 * Changes:
 * 09.09.2017:      initial version
 *
 *******************************************************************************************/

#include <LocoNet.h>
#include <EEPROM.h>
#include <OneButton.h>

#define  NCHAN         8           // number of detection channels (=number of addresses)
#define  START_DELAY   3000        // send "start of day" messages 3 secs after reset
#define  DEFAULTLNADDR  900         // first LN-Addr, if not set per LNCV[1] setting
#define  DEFAULTDELAY   500         // switch-off delay, if not set per LNCV[2] setting

#define  LEDBLINKTIME   350
#define  PROGLED 13
#define  PROGBUTTON 4   // !! LN Basis Board
#define  PROG_ACTIVATE_TIME  3000   // Tastendruck für 3 sekunden

//#define  EEPROMSIZE 2                  // 2 Byte vorsehen: Adresse und Abfallverzögerung

uint16_t debug;
const char swversion[] = "SW V01.02";
const char hwversion[] = "HW V01.00";
const char product[] = "LocoNetToti";
// article number: 51010 - only first 4 digits used
#define ARTNR   5101     // this is fixed and assigned to LN-Toti HW
#define LNCV_COUNT 4     // store 4 LNCVs
//uint16_t moduleAddr;
uint16_t lncv[LNCV_COUNT];   // values of LNCVs
LocoNetCVClass lnCV;

int LNAddr;
lnMsg *LnPacket;
int err;

// Pindefinitionen BASIS LocoNet-1.0
byte DataPin[8] = { 3, 7, 5, 9, 10, 11, 12, A0 };

byte DataRead[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // gelesene Daten
byte DataOut[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // zur Meldung a.d. Zentrale
byte lastDataOut[8] = { 2, 2, 2, 2, 2, 2, 2, 2 }; // letzte Meldung a.d. Zentrale
unsigned long lastOccupied[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // Zeitpunkt letzter Belegung

unsigned delayTime;       // Verzögerungszeit gegen Melderflackern

// für den Programmiervorgang über den LN-Bus:
boolean programmingModeEnabled = false; // key must be pressed to enter programming mode
boolean programmingMode = false;
OneButton progBtn(PROGBUTTON, true);  // 0 = key pressed
unsigned long blinkLEDtimer;
boolean ledON = false;
unsigned long debugTimer;

byte finalData;

void printLNCVs(void);
void debugLnPacket(void);
void finishProgramming(void);

//******************************************************************************************
void setup() {

	LocoNet.init();

	Serial.begin(57600);
	delay(10);

	EEPROM.get(0, lncv);   // read inital lncv values
	printLNCVs();

	debugTimer = millis();

	Serial.println(product);
	Serial.print("Art# ");
	uint32_t longArticleNumber = (uint32_t) ARTNR * 10;
	Serial.println(longArticleNumber);
	Serial.println(swversion);
	Serial.println(hwversion);

	for (int i = 0; i < 8; i++) {
		pinMode(DataPin[i], INPUT);
		digitalWrite(DataPin[i], HIGH);
		lastOccupied[i] = millis();
	}

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleEnableProgramming);
	progBtn.setPressTicks(PROG_ACTIVATE_TIME);

	LNAddr = lncv[1];
	if (LNAddr > 2047) {
		LNAddr = DEFAULTLNADDR;
		Serial.print(F("invalid LN-addr in lncv[1] - default "));
		Serial.print(DEFAULTLNADDR);
		Serial.println(F(" used"));
	}
	delayTime = lncv[2];
	if (delayTime > 2000) {
		delayTime = DEFAULTDELAY;
		Serial.print(F("invalid delay time in lncv[2] - default "));
		Serial.print(DEFAULTDELAY);
		Serial.println(F(" used"));
	}
	debug = lncv[3];

}

void printLNCVs() {
	Serial.println(F("LNCV#   Desc       =value"));
	Serial.print(F("0  Module Address = "));
	Serial.println(lncv[0]);
	Serial.print(F("1  LN StartAddr = "));
	Serial.println(lncv[1]);
	Serial.print(F("2  switch off Delay[ms] = "));
	Serial.println(lncv[2]);
	Serial.print(F("3  Debug(0=off,1=on,2=d-toti) = "));
	Serial.println(lncv[3]);
}

//******************************************************************************************
void toggleEnableProgramming() {
	// called when button is long pressed

	if (!programmingModeEnabled) {
		if (debug)
			Serial.println(F("enabled Progr. Mode"));
		ledON = true;
		digitalWrite(PROGLED, HIGH);
		blinkLEDtimer = millis();
		programmingModeEnabled = true;
	} else {
		finishProgramming();

	}
}

void finishProgramming() {
	programmingMode = false;
	programmingModeEnabled = false;
	ledON = false;
	digitalWrite(PROGLED, LOW);

	if (debug) {
		Serial.print(F("End Programing Mode.\n"));
		printLNCVs();
	}
}

//******************************************************************************************
void blinkLEDtick() {
	if ((millis() - blinkLEDtimer) > LEDBLINKTIME) {
		// toggle LED
		blinkLEDtimer = millis();
		if (ledON) {
			ledON = false;
			digitalWrite(PROGLED, LOW);
		} else {
			ledON = true;
			digitalWrite(PROGLED, HIGH);
		}
	}
}

/*** LOCONET PROTOCOL:
 *   [B2 61 72 5E]  Sensor LS708 () is High    
 *   [B2 61 62 4E]  Sensor LS708 () is Low
 *   <IN1> => A6=1, A5=1, A0=1 => addr1 = 1 + 32 + 64 = 97
 *   <IN2> => A8=1             => addr2 = 256  => LN_ADDR = 353
 *   
 *   OPC_INPUT_REP  0xB2
 ; General SENSOR Input codes
 ; <0xB2>, <IN1>, <IN2>, <CHK>

 <IN1> =<0,A6,A5,A4 - A3,A2,A1,A0>, 7 ls adr bits. A1,A0 select 1 of 4 inputs pairs in a DS54
 <IN2> =<0,X,I,L - A10,A9,A8,A7>
 Report/status bits and 4 MS adr bits.
 "I"=0 for DS54 "aux" inputs and 1 for "switch" inputs mapped to 4K SENSOR space.
 (This is effectively a least significant adr bit when using DS54 input configuration)
 "L"=0 for input SENSOR now 0V (LO) , 1 for Input sensor >=+6V (HI), "X"=1, control bit ,
 0 is RESERVED for future!
 */

//******************************************************************************************
void loop() {

	progBtn.tick();
	if (programmingMode) {            // process the received data from SX-bus
		blinkLEDtick();
	}

	// check sensor states and send update to LN, if changed
	for (int i = 0; i < 8; i++) {
		DataRead[i] = digitalRead(DataPin[i]); // active low Input: Data[i]==0 = Belegt!
		if (!DataRead[i]) { // --> belegt!
			DataOut[i] = DataRead[i];
			lastOccupied[i] = millis();
		} else {            // frei!
			if (millis() > lastOccupied[i] + delayTime) { // delayTime abgelaufen?
				DataOut[i] = DataRead[i];
			}
		}
		// send SOD after 3 secs after reset
		if (millis() > START_DELAY) {
			if (DataOut[i] != lastDataOut[i]) {
				lastDataOut[i] = DataOut[i];
				if (DataOut[i]) {
					LocoNet.reportSensor((uint16_t) (LNAddr + i), 0);
				} else {
					LocoNet.reportSensor((uint16_t) (LNAddr + i), 1);
				}
			}
		}
	}

	uint8_t packetConsumed;

	LnPacket = LocoNet.receive();

	if (LnPacket) {
		if (debug)
			debugLnPacket();

		packetConsumed = LocoNet.processSwitchSensorMessage(LnPacket);
		if ((packetConsumed == 0) && (programmingModeEnabled)) {
			packetConsumed = lnCV.processLNCVMessage(LnPacket);
		}

	}

	if (debug >= 2) {  // toti output to serial every second
		if (millis() > debugTimer + 1000) {
			debugTimer = millis();
			for (int i = 7; i >= 0; i--) {
				Serial.print(bitRead(finalData, i));
			}
			Serial.print("  ");
			Serial.println(finalData);
		}
	}
}   //loop

// The following routines are copied from LocoNetLNCVDemo
// Copyright (C) 2013 Damian Philipp
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
			finishProgramming;
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

