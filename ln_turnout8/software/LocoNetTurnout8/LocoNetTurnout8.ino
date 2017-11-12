/*
 * ******************************************
 * LocoNetTurnout8
 * 
 * Control 8 Tortoise turnouts via LocoNet
 * 
 * Michael Blank, 12.11.2017
 * 
 * Startup of LocoNet ?? initial state of turnouts ?
 * 
 */

#include <LocoNet.h>
#include <EEPROM.h>
#include <OneButton.h>

#include "Turnouts.h"
#include "lncv_prog.h"
#include "config.h"      // contains article number and lncv count

#define  NONUMBER    (10000)
#define  CLOSED   (0x30)   // used in LACK
#define  THROWN   (0x50)   // used in LACK
#define  DEFAULTLNADDR  801    // first LN-Addr, if not set per LNCV[1] setting

#define  LEDBLINKTIME   350
#define  PROGLED 13
#define  PROGBUTTON 4   // !! LN Basis Board
#define  PROG_ACTIVATE_TIME  3000   // Tastendruck für 3 sekunden

Turnouts turnouts(A4, A3, A2, A1, A0, 12, 11, 10);
#define  ENABLE_PIN  A5

uint16_t firstLNAddress = DEFAULTLNADDR;   //  first LN address.
uint8_t numberOfTurnouts = 4;  //actual number of turnouts, set by LNCV

uint16_t lastAddr, debug;
const char swversion[] = "SW V01.20";
const char hwversion[] = "HW V01.00";
const char product[] = "LocoNetTurnout8";

extern uint16_t lncv[];   // values of LNCVs
extern LocoNetCVClass lnCV;

lnMsg *LnPacket;
uint32_t time0, lasttime;

// programming of LNCVs must be enabled by pressing key for >= 3 seconds
extern boolean programmingModeEnabled;
extern boolean programmingMode;

OneButton progBtn(PROGBUTTON, true);   // key pressed == LOW
unsigned long blinkLEDtimer;
boolean ledON = false;
unsigned long debugTimer;

void printLNCVs(void);
void debugLnPacket(void);
void finishProgramming(void);

/*******************************************************************/
void setup() {
	// First initialize the LocoNet interface
	LocoNet.init();
	turnouts.init(ENABLE_PIN);

	// Configure the serial port for 57600 baud
	Serial.begin(57600);

	Serial.println(product);
	Serial.print("Art# ");
	uint32_t longArticleNumber = (uint32_t) ARTNR * 10;
	Serial.println(longArticleNumber);
	Serial.println(swversion);
	Serial.println(hwversion);
	time0 = millis();

	readLNCVsFromEEPROM();   // read initial lncv values
	printLNCVs();
	checkSettings();

	// try to read current state from LN

	for (int n = 0; n < numberOfTurnouts; n++) {
		LocoNet.reportSwitch(firstLNAddress + n);
	}

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleEnableProgramming);
	progBtn.setPressTicks(PROG_ACTIVATE_TIME);
}

void checkSettings() {
	firstLNAddress = lncv[1];
	if ((firstLNAddress <= 0) || (firstLNAddress > 2047)) {
		firstLNAddress = DEFAULTLNADDR;
		Serial.print(F("invalid LN-addr in lncv[1] - default "));
		Serial.print(DEFAULTLNADDR);
		Serial.println(F(" used"));
	}
	numberOfTurnouts = lncv[2];
	if ((numberOfTurnouts <= 0) || (numberOfTurnouts > 8)) {
		numberOfTurnouts = 8;
		Serial.print(F("invalid number of turnouts in lncv[2] - default "));
		Serial.print(8);
		Serial.println(F(" used"));
	}
	debug = lncv[3];
	if (debug) {
		Serial.println(F("Debug is on"));
	} else {
		Serial.println(F("Debug is off"));
	}
}

void printLNCVs() {
	Serial.println(F("LNCV#  Description = value"));
	Serial.print(F("0  Module Address = "));
	Serial.println(lncv[0]);
	Serial.print(F("1  LN StartAddr = "));
	Serial.println(lncv[1]);
	Serial.print(F("2  Number of Turnouts = "));
	Serial.println(lncv[2]);
	Serial.print(F("3  debug = "));
	Serial.println(lncv[3]);
}

//******************************************************************************************
void toggleEnableProgramming() {
	// called when button is long pressed

	if (!programmingModeEnabled) {
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

	Serial.print(F("End Programing Mode.\n"));
	printLNCVs();

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
void loop() {

	progBtn.tick();
	if (programmingMode) {            // process the received data from SX-bus
		blinkLEDtick();
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

}   // end loop

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Request messages
void notifySwitchRequest(uint16_t Address, uint8_t Output, uint8_t Direction) {
	Serial.print("Switch Request: ");
	Serial.print(Address, DEC);
	Serial.print(':');
	Serial.print(Direction ? 1 : 0);
	Serial.print(" (1=Cl 0=Thr) ");
	Serial.println(Output ? "On" : "Off");

	if (!Output)
		return;  // ignore "off command
	matchAndSetTurnout(Address, Direction);
}

bool matchAndSetTurnout(uint16_t a, uint8_t d) {
	if (d != 0)
		d = 1;

	for (int n = 0; n < numberOfTurnouts; n++) {
		if (a == (firstLNAddress + n)) {
			turnouts.set(n, d);
			Serial.print("set ");
			Serial.print(n);
			Serial.print("=");
			Serial.println(d);
			return true;
		}
	}
	return false;
}
// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Output Report messages
void notifySwitchOutputsReport(uint16_t Address, uint8_t ClosedOutput,
		uint8_t ThrownOutput) {
	Serial.print("Switch Outputs Report: ");
	Serial.print(Address, DEC);
	Serial.print(": Closed - ");
	Serial.print(ClosedOutput ? "On" : "Off");
	Serial.print(": Thrown - ");
	Serial.println(ThrownOutput ? "On" : "Off");
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

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch State messages
void notifySwitchState(uint16_t Address, uint8_t Output, uint8_t Direction) {
	Serial.print("Switch State: ");
	lastAddr = Address;  // for later user in LACK msg
	lasttime = millis(); // store time also
	Serial.print(Address, DEC);
	Serial.print(':');
	Serial.print(Direction ? "Closed" : "Thrown");
	Serial.print(" - ");
	Serial.println(Output ? "On" : "Off");
	//if (!Output)
	//		return;  // ignore "off command
	//matchAndSetTurnout(Address, Direction);
}

