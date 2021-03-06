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

#define N_CHAN  (8)

uint8_t out_pins[N_CHAN] = {A0, 12, 11, 10, 9,  5,  7,  3};
uint16_t addr[N_CHAN] = {0,0,0,0,0,0,0,0};

uint16_t lastAddr, debug;
bool startup = true;
uint8_t startupCount = 0, startupDelay = 10;

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
	LocoNet.init();  // uses pins 6 and 8

	for (uint8_t i= 0; i < N_CHAN; i++) {
		pinMode(out_pins[i], OUTPUT);
		digitalWrite(out_pins[i], HIGH);
	}

	// Configure the serial port for 57600 baud
	Serial.begin(115200);

	// some debug output to serial port
	Serial.println(F("LocoNetGpOut8"));
	Serial.print(F("Art# "));
	uint32_t longArticleNumber = (uint32_t) ARTNR * 10;
	Serial.println(longArticleNumber);
	Serial.println(F("SW V01.00"));
	Serial.println(F("HW V01.00"));

	time0 = millis();
	randomSeed(analogRead(6));

	initLncv();

	startupDelay = random(10,30);

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleEnableProgramming);
	progBtn.setPressTicks(PROG_ACTIVATE_TIME);

}

void initLncv() {
	// read initial lncv values
	readLNCVsFromEEPROM();
	printLNCVs();
	checkSettings();
	// init turnout loconet addresses
	for (int i = 0; i < N_CHAN; i++) {
		addr[i] = lncv[i+1];
	}

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

	for (uint8_t i= 1; i <= N_CHAN; i++) {
		Serial.print(F("lncv["));
		Serial.print(i);
		Serial.print(F("] : "));
		Serial.println(lncv[i]);
	}

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

	initLncv();  // read addresses
	startup = true;
	startupCount = 0;  // re-read state of address from loconet
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

	// after ten seconds, start with state requests for all used LN addresses
	if (startup && (millis() > (startupDelay * 1000)) ) {
		startupCount++;
		if (startupCount > LNCV_MAX_USED) {
			startup = false;
		}
		if ((lncv[startupCount] > 1) && (lncv[startupCount] < 2028)) {
			LocoNet.reportSwitch(lncv[startupCount]);
			delay(5);
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
	matchAndSetGPout(Address, Direction);

}

void matchAndSetGPout(uint16_t a, uint8_t d) {
	if (d != 0) {
		d = 1;
	}

	for (int i = 0; i < N_CHAN; i++) {
		if (a == addr[i]) {
			digitalWrite(out_pins[i], d);
			Serial.print("set out");
			Serial.print(i);
			Serial.print("=");
			Serial.println(d);
		}
	}
}


void notifyLongAck(uint8_t d1, uint8_t d2) {
	if (lastAddr != 0) {
		//Serial.print("addr=");
		//Serial.print(lastAddr);
		// Serial.print(" d1=");
		//Serial.print(d1,HEX);
		//Serial.print(" d2=");
		// Serial.println(d2,HEX);

		Serial.print(lastAddr);

		if (d2 & (uint8_t)0x40) {
			Serial.println(" closed");
			matchAndSetGPout(lastAddr,  0);
		} else {
			Serial.println(" thrown");
			matchAndSetGPout(lastAddr,  1);
		}
	}
	lastAddr=0; //reset to "unknown"
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
	Serial.println(Address, DEC);
	/* no info in the following bytes:
	Serial.print(':');
	//Serial.print(Direction ? "Closed" : "Thrown");
    Serial.print("dir=");
    Serial.print(Direction);
	Serial.print(" - ");
	//Serial.println(Output ? "On" : "Off");
    Serial.print("out=");
    Serial.println(Output);  */
}

