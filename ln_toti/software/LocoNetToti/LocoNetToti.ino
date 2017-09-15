/*******************************************************************************************
 * LocoNetToti 
 * (based on: SX_Belegtmelder_V0210)
 *
 * Created:    09.09.2017
 * Copyright:  Michael Blank
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
#define TESTVERSION  // aktiviert serielle Testausgaben und Dialog über Serial Interface
//#define DEBUG_TOTI   // aktiviert TOTI Output über Serial

#include <LocoNet.h>
#include <EEPROM.h>
#include <OneButton.h>

#define  DEFAULTADDR  890          // die Start-Adresse, auf die dieses Modul hört
#define  DEFAULTCODEDDELAYTIME 20  // *25ms = 500ms Abfallverzögerung
#define  DELAYTIMEFACTOR 25        // 1 digit entspricht 25ms
#define  START_DELAY   3000        // send "start of day" messages 3 secs after reset

#define  TESTTAKT 1000
#define  LEDTAKT 250
#define  PROGLED 13
#define  PROGBUTTON 4   // !! LN Basis Board
#define  PROG_ACTIVATE_TIME  3000   // Tastendruck für 3 sekunden

#define  EEPROMSIZE 2                  // 2 Byte vorsehen: Adresse und Abfallverzögerung

char swversion[] = "SW V01.02";
char hwversion[] = "HW V01.00";
char product[] = "LocoNetToti";
// article number: 51000 - only first 4 digits used
const uint16_t artno = 5100;  

int LNAddr;
lnMsg *LnPacket;

int err;

// Pindefinitionen BASIS SX RJ45-1.1
//byte DataPin[8] = { 5, 7, 8, 9, 10, 11, 12, A0 };

// Pindefinitionen BASIS LocoNet-1.0
byte DataPin[8] = { 3, 7, 5, 9, 10, 11, 12, A0 };

byte DataRead[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // gelesene Daten
byte DataOut[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // zur Meldung a.d. Zentrale
byte lastDataOut[8] = { 2, 2, 2, 2, 2, 2, 2, 2 }; // letzte Meldung a.d. Zentrale
unsigned long lastOccupied[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // Zeitpunkt letzter Belegung

unsigned delayTime;               // Verzögerungszeit gegen Melderflackern
int codedDelayTime;       // Abbildung in einem Byte: pro Digit=25ms, hier 500ms
//   dieser Wert wird im EEPROM abgelegt

// für den Programmiervorgang über den LN-Bus:
boolean programming = false;
OneButton progBtn(PROGBUTTON, true);  // 0 = key pressed
unsigned long blinkLEDtime;
boolean ledON = false;
unsigned long time1;


byte finalData;

//******************************************************************************************
void setup() {

	LocoNet.init();


	time1 = millis();
	Serial.begin(57600);
	delay(10);
  Serial.println(product);
  Serial.print("Art# ");
  Serial.println(artno*10);
	Serial.println(swversion);
	Serial.println(hwversion);


	for (int i = 0; i < 8; i++) {
		pinMode(DataPin[i], INPUT);
		digitalWrite(DataPin[i], HIGH);
		lastOccupied[i] = millis();
	}

  
	digitalWrite(PROGLED, LOW);
	pinMode(PROGLED, OUTPUT);

	progBtn.attachLongPressStart(toggleProgramming);
	progBtn.setPressTicks(PROG_ACTIVATE_TIME);

  LNAddr = DEFAULTADDR;
  delayTime = DEFAULTCODEDDELAYTIME * DELAYTIMEFACTOR;

  Serial.print("ln-adr=");
  Serial.print(LNAddr);
  Serial.println(" ..7");
  
	/*if (!EEPROMEmpty()) {
	 EEPROMRead();
	 }
	 else {
	 SXAddr = DEFAULTADDR;
	 codedDelayTime = DEFAULTCODEDDELAYTIME;
	 delayTime = codedDelayTime * DELAYTIMEFACTOR;
	 EEPROMSave();
	 } */

	// TODO init LocoNet
}



//******************************************************************************************
void toggleProgramming() {
	// called when button is long pressed

	if (!programming) {
		startProgramming();
	} else {
		finishProgramming();
	}
}

//******************************************************************************************
void startProgramming() {

#ifdef TESTVERSION
	Serial.println(F("startProgramming"));
#endif 
  ledON = true;
	digitalWrite(PROGLED, HIGH);
  blinkLEDtime = millis();
	programming = true;
	//saveOldSXValues();

	// TODO
}

//******************************************************************************************
void finishProgramming() {
 
#ifdef TESTVERSION
	Serial.println(F("finishProgramming"));
#endif
	/*SXAddr         = checkedSXtmp[0];
	 codedDelayTime = checkedSXtmp[1];

	 EEPROMSave();
	 EEPROMRead();
	 restoreOldSXValues();
	 delay(80); */
	programming = false;
  ledON = false;
	digitalWrite(PROGLED, LOW);
}

//******************************************************************************************
void processProgramming() {
// TODO
   if ((millis() - blinkLEDtime) >350) {
     // toggle LED
     blinkLEDtime = millis();
     if (ledON) {
      ledON = false;
      digitalWrite(PROGLED,LOW);
     } else {
      ledON = true;
      digitalWrite(PROGLED,HIGH);
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

	int x;

	progBtn.tick();
	if (programming) {            // process the received data from SX-bus
		processProgramming();
	}

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
    if (millis() > START_DELAY) { // send SOD after 3 secs after reset
                                 // after network interface has been started
		if (DataOut[i] != lastDataOut[i]) {
			lastDataOut[i] = DataOut[i];
      if (DataOut[i]) {
        LocoNet.reportSensor((uint16_t)(LNAddr+i), 0);
      } else {
        LocoNet.reportSensor((uint16_t)(LNAddr+i), 1);
      }
		}
   }

	}

	lnMsg *LnPacket;
	// TODO send to LN Bus

	LnPacket = LocoNet.receive();

	if (LnPacket) {

		unsigned char opcode = (int) LnPacket->sz.command;

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

   // If this packet was not a Switch or Sensor Message then print a new line
    if (!LocoNet.processSwitchSensorMessage(LnPacket)) {
      Serial.println();
    }

	}


#ifdef DEBUG_TOTI
	if (millis() > time1 + TESTTAKT) {
		time1 = millis();

		for (int i = 7; i >= 0; i--) {
			Serial.print(bitRead(finalData, i));
		}
		Serial.print("  ");
		Serial.println(finalData);
	}

#endif
}   //loop

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Sensor messages
void notifySensor( uint16_t Address, uint8_t State ) {
  Serial.print("notify Sensor: ");
  Serial.print(Address, DEC);
  Serial.print(" - ");
  Serial.println( State ? "Active" : "Inactive" );
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Switch Sensor Report messages
void notifySwitchReport( uint16_t Address, uint8_t State, uint8_t Sensor ) {
  Serial.print("Switch Sensor Report: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Sensor ? "Switch" : "Aux");
  Serial.print(" - ");
  Serial.println( State ? "Active" : "Inactive" );
}


