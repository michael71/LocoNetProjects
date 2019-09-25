/*
 * 
 * LN_MPX2
 * 
 * Schalten von Viessmann MPX Signalen über das LocoNet
 * 
 * basiert auf LocoNet Bibliothek, siehe
 *        https://github.com/mrrwa/LocoNet
 *
 * LocoNet receive pin=8, tx-pin=6
 *
 * Copyright (C) 2017 Michael Blank
 * 
 */

//#define HW_01       // BASIS_Loconet-1.0 Platine mit Aufsatz RT_Weiche_Signal, KEIN ULN
#define HW_02       // LN_Weiche_signalMPX 1.0
#define REV "REV_26.2.1018"

#include <LocoNet.h>
#include <OneButton.h>
// #include <EEPROM.h>  TODO: use LNCV programming

#include "mpx_signal.h"
#include "Turnouts.h"

#ifdef HW_02
#include "AnalogButtons.h"
#endif

#define DEBUG
#define NONUMBER    (10000)

#define CLOSED   (0x30)   // used in LACK
#define THROWN   (0x50)   // used in LACK


// loconet addresses of signals A and B (must bei different !)
#define N_LN   (3)


const uint16_t adrA[N_LN]   = {210, 211, 0};   // 2 ln addresses
const uint16_t adrB[N_LN]   = {212, 213, 0};   // 2 ln addresses
const uint16_t adrT[N_TURNOUTS] = {140, 141, 142, 143};   // 4 ln addresses for 4 turnouts

// -------------------- Hardware Definitions --- BASIS_LocoNet_1.0 + RT-weiche-signalmpx --
#ifdef HW_01
MPX_Signal sigA(A0, 12, 11, 10);   // signal A, pins { A0, 12, 11, 10} - see mpx_signal.h
MPX_Signal sigB(9, 5, 7, 3);   // signal B, pins { 9, 5, 7, 3} - see mpx_signal.h

Turnouts turnouts(A4, A3, A2, A1);
const uint8_t enablePin = A5;

#elif defined HW_02
// -------------------- Hardware Definitions --- LN_Weiche_SignalMPX ----------------------
MPX_Signal sigA(A0, 12, 11, 10);   // signal A, pins { A0, 12, 11, 10} - see mpx_signal.h
MPX_Signal sigB(9, 5, 7, 3);   // signal B, pins { 9, 5, 7, 3} - see mpx_signal.h

Turnouts turnouts(A4, A3, A2, A1);
const uint8_t enablePin = 2;

AnalogButtons aButtons = AnalogButtons(A5);

#else
#error "check hardware setting"
#endif

// -------------------- Hardware Definitions END ------------------------------------------

lnMsg        *LnPacket;

uint16_t lastadr = NONUMBER;
uint8_t  laststate = CLOSED;
uint32_t lasttime = 0, myTimer = 0;

OneButton button(4, true);

void setup() {
	// First initialize the LocoNet interface
	LocoNet.init();
	sigA.init();
	sigB.init();
	turnouts.init(enablePin);
	// Configure the serial port for 57600 baud
	Serial.begin(57600);
	Serial.print(F("LN_MPX2 Debug -"));
	Serial.println(F(REV));
	pinMode(13,OUTPUT);

	// aktuellen Zustand herausfinden

	for (int n=0; n <N_LN; n++) {
		if (adrA[n] != 0)
		  LocoNet.reportSwitch((uint16_t)adrA[n]);
	}

	for (int n=0; n <N_LN; n++) {
		if (adrB[n] != 0)
			LocoNet.reportSwitch((uint16_t)adrB[n]);
	}

	for (int n=0; n <N_TURNOUTS; n++) {
			if (adrT[n] != 0)
				LocoNet.reportSwitch((uint16_t)adrT[n]);
		}

	button.attachClick(toggleTurnout);

}



void toggleTurnout() {
	Serial.print("toggle turnout");
    uint8_t dir = turnouts.get(0);
    if (dir == 0) {
    	dir = 1;
    } else {
    	dir = 0;
    }

    setLNTurnout(adrT[0], dir);
}

void loop() {
	if ((millis() - myTimer) >= 2) {
		sigA.process();
		sigB.process();
		button.tick();  // check button state
		myTimer = millis();
	}
	// Check for any received LocoNet packets
	LnPacket = LocoNet.receive() ;
	if ( LnPacket ) {
		unsigned char opcode = (int)LnPacket->sz.command;

		// First print out the packet in HEX
		Serial.print("RX: ");
		uint8_t msgLen = getLnMsgSize(LnPacket);
		for (uint8_t x = 0; x < msgLen; x++)
		{
			uint8_t val = LnPacket->data[x];
			// Print a leading 0 if less than 16 to make 2 HEX digits
			if (val < 16)
				Serial.print('0');

			Serial.print(val, HEX);
			Serial.print(' ');
		}

		// If this packet was not a Switch or Sensor Message then print a new line
		if (!LocoNet.processSwitchSensorMessage(LnPacket)) {
			Serial.println();
		}

		if (opcode == OPC_GPON)  {
			Serial.println(F("Power ON"));
		} else if (opcode == OPC_GPOFF) {
			Serial.println(F("Power OFF"));
		} else if (opcode == OPC_IDLE) {
			Serial.println(F("EStop!"));
		} else if (opcode == OPC_SW_STATE) {
			uint16_t a = (LnPacket->srq.sw1 | ( ( LnPacket->srq.sw2 & 0x0F ) << 7 )) +1 ;
			Serial.print (F(" requesting switch state: "));
			Serial.println(a, DEC);
		} else if ((opcode == OPC_LONG_ACK) && (LnPacket->data[1] == 0x3C) ) {
			if ((millis() - lasttime) < 300) {
				Serial.print(F("LACK to SW_STATE req "));
				uint8_t Direction = LnPacket->data[2];
				uint8_t dir = 0;
				if (Direction == CLOSED) {
					Serial.print(F("closed - a="));
					Serial.println(lastadr);
					dir = 1;
				} else if (Direction == THROWN) {
					Serial.print(F("thrown - a="));
					Serial.println(lastadr);
					dir = 0;
				} else {
					Serial.print(F("Direction=Unknown - a="));
					Serial.println(lastadr);
				}
				matchAndSetSignal(lastadr, dir);  // sigA and sigB
				matchAndSetTurnouts(lastadr, dir);  // turnouts

			} else {
				Serial.println(F("unknown LACK to SW_STATE req "));
			}
		}

	}  // endif LnPacket

}   // end loop


// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Sensor messages
void notifySensor( uint16_t Address, uint8_t State ) {
	Serial.print(F("Sensor: "));
	Serial.print(Address, DEC);
	Serial.print(" - ");
	Serial.println( State ? "Active" : "Inactive" );
}

void print_sigA() {
	Serial.print(F("SigA val="));
	Serial.println(sigA.getln());


}
// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Request messages
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
	Serial.print(F("Switch Request: "));
	Serial.print(Address, DEC);
	Serial.print(':');
	uint8_t dir = Direction ? 1: 0;
	Serial.print(dir);
	Serial.print(F(" (1=Cl 0=Thr) "));
	Serial.println(Output ? "On" : "Off");

	if (!Output) return;  // ignore "off command
	matchAndSetSignal(Address, dir);
	matchAndSetTurnouts(Address, dir);  // turnouts
}

bool matchAndSetSignal (uint16_t a, uint8_t d) {
	for (int n=0; n<N_LN; n++) {
		// check, if one of the addresses of signalA is affected
			if (a == adrA[n]) {
				sigA.setln(n, d);
				return true;
			}  else if (a == adrB[n]) {
				sigB.setln(n, d);
				return true;
			}
	}
	return false;
}

bool matchAndSetTurnouts (uint16_t a, uint8_t d) {
	for (uint8_t n=0; n<N_TURNOUTS; n++) {
		// check, if one of the addresses of the turnouts affected
			if (a == adrT[n]) {
				turnouts.set(n, d);   // set turnout "n" to value "d"
#ifdef DEBUG
				Serial.print(F(" set T a="));
				Serial.print(a);
				Serial.print(F(" d="));
				Serial.println(d);
#endif
				return true;
			}
	}
	return false;
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Output Report messages
void notifySwitchOutputsReport( uint16_t Address, uint8_t ClosedOutput, uint8_t ThrownOutput) {
	Serial.print(F("Switch Outputs Report: "));
	Serial.print(Address, DEC);
	Serial.print(F(": Closed - "));
	Serial.print(ClosedOutput ? "On" : "Off");
	Serial.print(F(": Thrown - "));
	Serial.println(ThrownOutput ? "On" : "Off");
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Sensor Report messages
void notifySwitchReport( uint16_t Address, uint8_t State, uint8_t Sensor ) {
	Serial.print(F("Switch Sensor Report: "));
	Serial.print(Address, DEC);
	Serial.print(':');
	Serial.print(Sensor ? "Switch" : "Aux");
	Serial.print(" - ");
	Serial.println( State ? "Active" : "Inactive" );
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch State messages
void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) {
	Serial.print("Switch State: ");
	lastadr = Address;  // for later user in LACK msg
	lasttime = millis(); // store time also
	Serial.print(Address, DEC);
	Serial.print(':');
	Serial.print(Direction ? "Closed" : "Thrown");
	Serial.print(" - ");
	Serial.println(Output ? "On" : "Off");
}

void setLNTurnout(int address, byte dir) {
    sendOPC_SW_REQ( address - 1, dir, 1 );
    sendOPC_SW_REQ( address - 1, dir, 0 );
}

void sendOPC_SW_REQ(int address, byte dir, byte on) {
    lnMsg SendPacket ;

    int sw2 = 0x00;
    if (dir) sw2 |= B00100000;
    if (on)  sw2 |= B00010000;
    sw2 |= (address >> 7) & 0x0F;

    SendPacket.data[ 0 ] = OPC_SW_REQ ;
    SendPacket.data[ 1 ] = address & 0x7F ;
    SendPacket.data[ 2 ] = sw2 ;
    Serial.println(F("TX OPC_SW_REQ"));
    LocoNet.send( &SendPacket );
}

