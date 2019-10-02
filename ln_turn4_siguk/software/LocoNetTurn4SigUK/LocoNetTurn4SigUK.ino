/*
 * ******************************************
 * LocoNetTurn4SigUK
 * 
 * Control 4 Tortoise turnouts and 2 UK Signals via LocoNet
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

#define  NONUMBER    (10000)
#define  CLOSED   (0x30)   // used in LACK
#define  THROWN   (0x50)   // used in LACK

#define  LEDBLINKTIME   350
#define  PROGLED 13
#define  PROGBUTTON 4   // !! LN Basis Board
#define  PROG_ACTIVATE_TIME  3000   // Tastendruck für 3 sekunden

// Signal sigA(A0, 12, 11, 10);   // signal A, pins { A0, 12, 11, 10} - see mpx_signal.h
// Signal sigB(9, 5, 7, 3);   // signal B, pins { 9, 5, 7, 3} - see mpx_signal.h

#define N_TURNOUT (4)   //number of turnouts

Turnouts turnouts(A4, A3, A2, A1);
#define ENABLE_PIN  (A5)

uint16_t t_addr[N_TURNOUT] = {1,2,3,4};

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
	turnouts.init(ENABLE_PIN);

	// Configure the serial port for 57600 baud
	Serial.begin(57600);

	Serial.println(F("LocoNetTurno4SigUK"));
	Serial.print("Art# ");
	uint32_t longArticleNumber = (uint32_t) ARTNR * 10;
	Serial.println(longArticleNumber);
	Serial.println(F("SW V01.00"));
	Serial.println(F("HW V01.00"));
	time0 = millis();

	//set timer2 interrupt at 8kHz
	TCCR2A = 0;// set entire TCCR2A register to 0
	TCCR2B = 0;// same for TCCR2B
	TCNT2  = 0;//initialize counter value to 0
	// set compare match register for 8khz increments
	OCR2A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
	// turn on CTC mode
	TCCR2A |= (1 << WGM21);
	// Set CS21 bit for 8 prescaler
	TCCR2B |= (1 << CS21);
	// enable timer compare interrupt
	TIMSK2 |= (1 << OCIE2A);

	sei();//allow interrupts

	readLNCVsFromEEPROM();   // read initial lncv values
	printLNCVs();
	checkSettings();

	// try to read current state from LN

	for (int n = 0; n < N_TURNOUT; n++) {
		LocoNet.reportSwitch(t_addr[n]);
	}

	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleEnableProgramming);
	progBtn.setPressTicks(PROG_ACTIVATE_TIME);

}

void checkSettings() {
	for (int n = 0; n < N_TURNOUT; n++
	) {
		t_addr[n] = lncv[1+n];
		if ((t_addr[n] <= 0) || (t_addr[n] > 2047)) {
			Serial.print(F("invalid LN-addr in lncv[1] - default 1 used "));
		}
	}

	debug = lncv[LNCV_DEBUG];
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
	Serial.print(F("1  turnout1 address = "));
	Serial.println(lncv[1]);
	Serial.print(F("2  turnout2 address = "));
	Serial.println(lncv[2]);
	Serial.print(F("3  turnout3 address = "));
	Serial.println(lncv[3]);
	Serial.print(F("4  turnout4 address = "));
	Serial.println(lncv[4]);
	Serial.print(LNCV_DEBUG);
	Serial.print(F("  debug = "));
	Serial.println(lncv[LNCV_DEBUG]);
}

ISR(TIMER2_COMPA_vect){//timer2 interrupt 8kHz
  // TODO
	static uint16_t icount = 0;
	icount++;
	/* testcode
	static uint8_t toggle = 0;
	if (icount >= 8000) {
		icount = 0;
		if (toggle) {
			toggle = 0;
			digitalWrite(PROGLED, LOW);
		} else {
			toggle = 1;
			digitalWrite(PROGLED, HIGH);
		}
	} end testcode */
}


//*****************************************************************************************/
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
	if (programmingMode) { // process the received data from SX-bus
		blinkLEDtick();
	}

	uint8_t packetConsumed;

	LnPacket = LocoNet.receive();

	if (LnPacket) {
		if (debug)
			debugLnPacket();

		packetConsumed = LocoNet.processSwitchSensorMessage(
				LnPacket);
		if ((packetConsumed == 0) && (programmingModeEnabled)) {
			packetConsumed = lnCV.processLNCVMessage(LnPacket);
		}

	}

}   // end loop

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Request messages
void notifySwitchRequest(uint16_t Address, uint8_t Output,
		uint8_t Direction) {
	Serial.print("Switch Request: ");
	Serial.print(Address, DEC);
	Serial.print(':');
	Serial.print(Direction ? 1 : 0);
	Serial.print(" (1=Cl 0=Thr) ");
	Serial.println(Output ? "On" : "Off");

	if (!Output)
		return;  // ignore all off commands
	matchAndSetTurnout(Address, Direction);
}

bool matchAndSetTurnout(uint16_t a, uint8_t d) {
	if (d != 0)
		d = 1;

	for (int n = 0; n < N_TURNOUT; n++
	) {
		if (a == t_addr[n]) {
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
void notifySwitchOutputsReport(uint16_t Address,
		uint8_t ClosedOutput, uint8_t ThrownOutput) {
	Serial.print("Switch Outputs Report: ");
	Serial.print(Address, DEC);
	Serial.print(": Closed - ");
	Serial.print(ClosedOutput ? "On" : "Off");
	Serial.print(": Thrown - ");
	Serial.println(ThrownOutput ? "On" : "Off");
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Sensor Report messages
void notifySwitchReport(uint16_t Address, uint8_t State,
		uint8_t Sensor) {
	Serial.print("Switch Sensor Report: ");
	Serial.print(Address, DEC);
	Serial.print(':');
	Serial.print(Sensor ? "Switch" : "Aux");
	Serial.print(" - ");
	Serial.println(State ? "Active" : "Inactive");
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch State messages
void notifySwitchState(uint16_t Address, uint8_t Output,
		uint8_t Direction) {
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
