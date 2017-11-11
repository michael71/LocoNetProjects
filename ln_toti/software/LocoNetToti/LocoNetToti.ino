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
 * Adresse des Moduls und Abfallverzögerung werden im EEPROM des Controllers
 * abgespeichert.
 * lncv[0] contains Module Address
 * lncv[1] first LocoNetAddress
 * lncv[2] switch off Delay in milliseconds
 * lncv[3] Debug setting, 0=off, 1=on, 2=debug-toti
 *
 *
 * Using the LocoNet Arduino library, see https://github.com/mrrwa
 * 
 * Changes:
 * 16.09.2017:      using LNCV programming for parameters
 * 09.09.2017:      initial version
 *
 *******************************************************************************************/

#include <LocoNet.h>
#include <OneButton.h>
#include "lncv_prog.h"
#include "config.h"      // contains article number and lncv count

#define  NCHAN         8       // number of detection channels (=number of addresses)
#define  START_DELAY   3000    // send "start of day" messages 3 secs after reset
#define  DEFAULTLNADDR  901    // first LN-Addr, if not set per LNCV[1] setting
#define  DEFAULTDELAY   350    // switch-off delay, if not set per LNCV[2] setting

#define  LEDBLINKTIME   350
#define  PROGLED 13
#define  PROGBUTTON 4   // !! LN Basis Board
#define  PROG_ACTIVATE_TIME  3000   // Tastendruck für 3 sekunden

//#define  EEPROMSIZE 2                  // 2 Byte vorsehen: Adresse und Abfallverzögerung

uint16_t debug;
const char swversion[] = "SW V01.11";
const char hwversion[] = "HW V01.00";
const char product[] = "LocoNetToti";

extern uint16_t lncv[];   // values of LNCVs
extern LocoNetCVClass lnCV;

int firstLNAddr;
lnMsg *LnPacket;
int err;

// Pindefinitionen BASIS LocoNet-1.0
byte DataPin[8] = { 3, 7, 5, 9, 10, 11, 12, A0 };

byte DataRead[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // gelesene Daten
byte DataOut[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // zur Meldung a.d. Zentrale
byte lastDataOut[8] = { 2, 2, 2, 2, 2, 2, 2, 2 }; // letzte Meldung a.d. Zentrale
unsigned long lastOccupied[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // Zeitpunkt letzter Belegung

unsigned delayTime;       // Verzögerungszeit gegen Melderflackern

// programming of LNCVs must be enabled by pressing key for >= 3 seconds
extern boolean programmingModeEnabled;
extern boolean programmingMode;

OneButton progBtn(PROGBUTTON, true);   // key pressed == LOW
unsigned long blinkLEDtimer;
boolean ledON = false;
unsigned long debugTimer;

byte finalData;

void printLNCVs(void);
void debugLnPacket(void);
void finishProgramming(void);

//******************************************************************************************
void setup() {

	Serial.begin(57600);

	Serial.println(product);
	Serial.print("Art# ");
	uint32_t longArticleNumber = (uint32_t) ARTNR * 10;
	Serial.println(longArticleNumber);
	Serial.println(swversion);
	Serial.println(hwversion);

	LocoNet.init();

	readLNCVsFromEEPROM();   // read initial lncv values
	printLNCVs();
	checkSettings();

	debugTimer = millis();

	for (int i = 0; i < 8; i++) {
		pinMode(DataPin[i], INPUT);
		digitalWrite(DataPin[i], HIGH);
		lastOccupied[i] = millis();
	}

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleEnableProgramming);
	progBtn.setPressTicks(PROG_ACTIVATE_TIME);

}

void checkSettings() {
  Serial.print(F("checkSettings()"));
	firstLNAddr = lncv[1];
	if ((firstLNAddr > 2047) || (firstLNAddr <= 0)) {
		firstLNAddr = DEFAULTLNADDR;
		Serial.print(F("invalid LN-addr in lncv[1] - default "));
		Serial.print(DEFAULTLNADDR);
		Serial.println(F(" used"));
	} else {
    Serial.print(F("LN-addr in lncv[1]= "));
    Serial.println(firstLNAddr);
	}
	delayTime = lncv[2];
	if ((delayTime > 2000) || (delayTime <= 0)) {
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
	checkSettings();
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

//*************************************************************************
void loop() {

	progBtn.tick();
	if (programmingMode) {            // process the received data from SX-bus
		blinkLEDtick();
	}

	// check sensor states and send update to LN, if state has changed
	for (int i = 0; i < 8; i++) {
		DataRead[i] = digitalRead(DataPin[i]);
		// active low Input: Data[i]==0 = occupied!
		if (!DataRead[i]) { // --> belegt!
			DataOut[i] = DataRead[i];
			lastOccupied[i] = millis();
		} else {            // frei!
			if (millis() > lastOccupied[i] + delayTime) {
				// delayTime abgelaufen?
				DataOut[i] = DataRead[i];
			}
		}
		// send SOD after 3 secs after reset
		if (millis() > START_DELAY) {
			if (DataOut[i] != lastDataOut[i]) {
				lastDataOut[i] = DataOut[i];
				if (DataOut[i]) {
					LocoNet.reportSensor((uint16_t) (firstLNAddr + i), 0);
				} else {
					LocoNet.reportSensor((uint16_t) (firstLNAddr + i), 1);
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

