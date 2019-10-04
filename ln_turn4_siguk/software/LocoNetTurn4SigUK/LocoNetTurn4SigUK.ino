/*
 * ******************************************
 * LocoNetTurn4SigUK
 * 
 * Control 4 Tortoise turnouts plus
 * 2 UK Signals (with feather) via LocoNet
 * 
 * Michael Blank, 2.10.2019
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
#include "Signal.h"

#define  NONUMBER    (10000)
#define  CLOSED   (0x30)   // used in LACK
#define  THROWN   (0x50)   // used in LACK
#define  DEFAULTLNADDR  801    // first LN-Addr, if not set per LNCV[1] setting

#define  LEDBLINKTIME   350
#define  PROGLED 13
#define  PROGBUTTON 4   // !! LN Basis Board
#define  PROG_ACTIVATE_TIME  3000   // Tastendruck für 3 sekunden

Turnouts turnouts(A4, A3, A2, A1);
#define  ENABLE_PIN  A5


#define N_TURNOUT  (4)
uint16_t t_addr[N_TURNOUT];

Signal sig1(0);
Signal sig2(1);
uint16_t sig1_addr[3] = {0,0,0};
uint16_t sig1_feath_inv = 0;
uint16_t sig2_addr[3] = {0,0,0};
uint16_t sig2_feath_inv = 0;

uint16_t lastAddr, debug;

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

	// then turnout and signal outputs
	turnouts.init(ENABLE_PIN);
	sig1.init();
	sig2.init();

	// Configure the serial port for 57600 baud
	Serial.begin(115200);

	// some debug output to serial port
	Serial.println(F("LocoNetTurn4SigUK"));
	Serial.print(F("Art# "));
	uint32_t longArticleNumber = (uint32_t) ARTNR * 10;
	Serial.println(longArticleNumber);
	Serial.println(F("SW V01.00"));
	Serial.println(F("HW V01.00"));

	time0 = millis();

	// read initial lncv values
	readLNCVsFromEEPROM();
	printLNCVs();
	checkSettings();

	// init turnout loconet addresses
	// try to read current state from LN
	for (int i = 0; i < N_TURNOUT; i++) { 
        t_addr[i] = lncv[i+1];
		LocoNet.reportSwitch(t_addr[i]);
	}

	sig1_addr[0] = lncv[5];
	sig1_addr[1] = lncv[6];   // set to 0 for signal with only 2 aspects
	sig1_addr[2] = lncv[7];   // set to 0 for signal without feather
	sig1_feath_inv = lncv[8];

	sig2_addr[0] = lncv[9];
	sig2_addr[1] = lncv[10];
	sig2_addr[2] = lncv[11];
	sig2_feath_inv = lncv[12];

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleEnableProgramming);
	progBtn.setPressTicks(PROG_ACTIVATE_TIME);
}

void checkSettings() {

    for (uint8_t i=0; i <= LNCV_MAX_USED ; i++) {
        if ((lncv[i] < 0) || (lncv[i] > 2047)) {
		   Serial.print(F("invalid LN-addr in lncv["));
           Serial.print(i);
           Serial.println(F("]"));
	   }
	}	
}

void printLNCVs() {
	Serial.println(F("LNCV#  Description value"));
	Serial.print(F("0  Module Address: "));
	Serial.println(lncv[0]);

	Serial.print(F("1-4   turnouts: "));
	Serial.print(lncv[1]);
	Serial.print(',');
	Serial.print(lncv[2]);
	Serial.print(',');
	Serial.print(lncv[3]);
	Serial.print(',');
	Serial.println(lncv[4]);

	Serial.print(F("5,6   sig1: "));
	Serial.print(lncv[5]);
	Serial.print(',');
	Serial.println(lncv[6]);

    Serial.print(F("7,8   sig1 feather: "));
	Serial.print(lncv[7]);
	Serial.print(',');
	Serial.println(lncv[8]);

	Serial.print(F("9,10  sig2: "));
	Serial.print(lncv[9]);
	Serial.print(',');
	Serial.println(lncv[10]);
 
    Serial.print(F("11,12 sig2 feather: "));
	Serial.print(lncv[11]);
	Serial.print(',');
	Serial.println(lncv[12]);

    Serial.println(F("13,14 not used."));

	Serial.print(LNCV_DEBUG);   // 15
	Serial.print(F("    debug = "));
	Serial.println(lncv[LNCV_DEBUG]);
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

	//if (!Output)
	//	return;  // ignore "off command
	matchAndSetTurnout(Address, Direction);
	matchAndSetSignals(Address, Direction);
}

void matchAndSetTurnout(uint16_t a, uint8_t d) {
	if (d != 0) {
		d = 1;
	}

	for (int i = 0; i < N_TURNOUT; i++) {
		if (a == t_addr[i]) {
			turnouts.set(i, d);
			Serial.print("set T[");
			Serial.print(i);
			Serial.print("]=");
			Serial.println(d);
		}
	}
}


 void matchAndSetSignals(uint16_t a, uint8_t d) {

	if (d != 0) {
		d = 1;
	}

	if (a == sig1_addr[0]) {
		sig1.setRG(d);
	} else if (a == sig1_addr[1]) {
		sig1.setY(d);
	} else if (a ==   sig1_addr[2]) {
		sig1.setFeather(d);
	}

	if (a == sig2_addr[0]) {
		sig2.setRG(d);
	} else if (a == sig2_addr[1]) {
		sig2.setY(d);
	} else if (a ==   sig2_addr[2]) {
		sig2.setFeather(d);
	}
	return;
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
}

