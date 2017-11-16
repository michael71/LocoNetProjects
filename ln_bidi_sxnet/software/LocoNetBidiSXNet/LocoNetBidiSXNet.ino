/****************************************************************************
 * 	Copyright (C) 2017 Michael Blank
 * 
 * 	This library is free software; you can redistribute it and/or
 * 	modify it under the terms of the GNU Lesser General Public
 * 	License as published by the Free Software Foundation; either
 * 	version 2.1 of the License, or (at your option) any later version.
 * 
 * 	This library is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * 	Lesser General Public License for more details.
 * 
 * 	You should have received a copy of the GNU Lesser General Public
 * 	License along with this library; if not, write to the Free Software
 * 	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *****************************************************************************
 * 
 * Title :   LocoNetBidiSXNet
 * Author:   Michael Blank
 * Date:     12-Nov-2017
 * Software: AVR-GCC
 * Target:   Arduino
 * 
 * 
 * DESCRIPTION
 *   1) sends all sensor messages (received from LocoNet) in an address range from
 *   851 to 1041 to SXnet 85 to 104 
 *   channel 85, sxbis1 = 851 ...
 *   channel 85, sxbit8 = 858 etc 
 *   2) sends SXnet messages "X adr data" to LocoNet (for turnouts) for specific 
 *   addresses
 * 
 *   The Loconet RX is on arduino pin 8, the tx on arduino pin 6.
 *   
 *   Hardware: Arduino (Uno) Ethernet + FREMO LN-Shield
 *   
 *   startup: sensor boards send there state a few seconds after a reset
 *   
 *****************************************************************************/

#include <LocoNet.h>
#include <SPI.h>
#include <Ethernet.h>

#define DEBUG /* enable this to print debug information via the serial port */

/* This is the TCP port used to communicate over ethernet */

#define TCP_PORT 4104
#define UPDATE_INTERVAL  200    // send update every 200 msec
uint32_t lastUpdate = 0;

/* Modify these variables to match your network */
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xAA, 0xED };
//const byte ip[] = { 192, 168, 178, 100 };
//const byte gateway[] = { 192, 168, 178, 1 };
//const byte subnet[] = { 255, 255, 255, 0 };

// retry connection, if not connected, after RETRY_INTERVAL milliseconds
#define RETRY_INTERVAL   30000
uint32_t lastRetry = 0;

const char digitMap[] = "0123456789abcdef";
const char receiveString[] = "RECEIVE ";
static LnBuf LnTxBuffer;
static lnMsg *LnPacket;

// buffer for received messages from SXNet "X 33 12" (channel 33 was set to 12)
#define SXRECBUF_LEN   40
char sxRecBuffer[SXRECBUF_LEN];
uint8_t sxRecBufPos;

#define N_REC_CHAN  4   // listen to 1 channel on SXNet and transfer them to LN
const String turnout_cmd[N_REC_CHAN] = { "X 91", "X 92", "X 93", "X 94" };
const int firstTurnoutLNAddress[N_REC_CHAN] = { 911, 921, 931, 941};
const uint8_t len_turnout_cmd[N_REC_CHAN] = { 4, 4, 4, 4}; //turnout_cmd.length();
uint8_t startUp[N_REC_CHAN] = { 1, 1, 1, 1}; // set all turnout at startup of communication
// and request all turnout channels every 30 secs  
uint32_t turnoutRequestTime[N_REC_CHAN] = { 0, 0, 0, 0};

int recChanData[N_REC_CHAN];
int oldRecChanData[N_REC_CHAN] = { -1, -1, -1, -1};

EthernetClient client;
IPAddress server;

#define MIN_SX_ADDR   85
#define MAX_SX_ADDR   104
#define LN_MULTIPLIER  10   // LN_ADDR = SX_ADDR + iBit   88,bit1 == 881  FIRST LN_ADDR MUST bei XX1 !!
#define SENSOR_COMM
byte sxData[MAX_SX_ADDR - MIN_SX_ADDR + 1]; //sx addresses 80 (=[0]) to 104 [24]
boolean sxChanged[MAX_SX_ADDR - MIN_SX_ADDR + 1]; //sx addresses 80 (=[0]) to 104 [24]

void connectToSXnet() {
	/* SXnet defaults to PORT 4104 */
	// if you get a connection, report back via serial:
	if (client.connect(server, TCP_PORT)) {
		Serial.println("connected");
	} else {
		// kf you didn't get a connection to the server:
		Serial.println("connection failed");
	}
}

String IpAddress2String(const IPAddress& ipAddress) {
	return String(ipAddress[0]) + String(".") + String(ipAddress[1])
			+ String(".") + String(ipAddress[2]) + String(".")
			+ String(ipAddress[3]);
}

void setup() {
	/* First initialize the LocoNet interface */
	LocoNet.init();
 
  /* initialise the ethernet device (DHCP) */
  Ethernet.begin(mac);

  // figure out if we use home network or ibmklub network
  String myIP = IpAddress2String(Ethernet.localIP());
  if (myIP.indexOf("192.168.178") == 0) {
    // home ip range
    server = IPAddress(192, 168, 178, 29);  // home sx3 server
  } else {
    // ibm klub ip range
    server = IPAddress(192, 168, 1, 10);  // ibmklub sx3 server
  }
  
	/* Configure the serial port for 57600 baud */
	Serial.begin(57600);
	Serial.println(F("LocoNet BiDi <-> SXnet started!!"));
	Serial.print(F("loconet sensor addresses must be in range "));
	Serial.print(MIN_SX_ADDR * LN_MULTIPLIER + 1);
	Serial.print(" to ");
	Serial.println(MAX_SX_ADDR * LN_MULTIPLIER + 1);
  Serial.print(F("My address="));
  Serial.println(myIP);
	Serial.print(F("Server address="));
	String sIP = IpAddress2String(server);
	Serial.println(sIP);
  Serial.print(F("Turnout sx-addr="));
  for (uint8_t i=0; i <N_REC_CHAN; i++) {
	    Serial.print(turnout_cmd[i].substring(2));
      if (i != (N_REC_CHAN -1)) Serial.print(", ");
  }
  Serial.println();

	/*Initialize a LocoNet packet buffer to buffer bytes from the PC */
	initLnBuf(&LnTxBuffer);

	sxRecBuffer[0] = '\0';
	sxRecBufPos = 0;

	/* connect */
	connectToSXnet();

}

/* set an LN turnout, if the data bit has changed OR in startup phase
 *  
 */
void setLNTurnouts(int firstA, int d, int oldD, uint8_t stUP) {

  for (uint8_t bit = 0; bit < 8; bit++) {
    uint8_t mask = (1 << bit);
    if ((stUP) || ((d & mask) != (oldD & mask))) {
      int a = firstA + bit;
      if (d & mask) {
         setLNTurnout(a, 1);
         Serial.print(F("set LNAddr"));
         Serial.print(a);
         Serial.println("=1");
      } else {
         setLNTurnout(a, 0);
         Serial.print(F("set LNAddr"));
         Serial.print(a);
         Serial.println("=0");
      }
    }
  }
}

void requestTurnoutStates() {
  for (uint8_t i = 0; i < N_REC_CHAN; i++) {
    if ( ((millis() - turnoutRequestTime[i] ) > 30000)       
         && client.connected()) {
         client.print("R ");
         client.println(turnout_cmd[i].substring(2));
         Serial.print("R ");
         Serial.println(turnout_cmd[i].substring(2)); 
         turnoutRequestTime[i] = millis(); 
    }
  }
}   

void loop() {

	if (client.available()) {
		// get incoming byte:
		char c = client.read();

		if (c != '\n') {
			sxRecBuffer[sxRecBufPos] = c;
			if (sxRecBufPos < SXRECBUF_LEN - 1)
				sxRecBufPos++;

		} else { //command completed
			sxRecBuffer[sxRecBufPos] = 0;
			String s(sxRecBuffer);
      Serial.println(s);
			sxRecBuffer[0] = '\0';  // reset buffer
			sxRecBufPos = 0;
      // check, if this command is for one of "our" LN Turnouts
      for (uint8_t i = 0; i < N_REC_CHAN; i++) {
        if (s.startsWith(turnout_cmd[i])) {
				  // matching address, get value
				  recChanData[i] = s.substring(len_turnout_cmd[i]).toInt();
				  if (recChanData[i] != oldRecChanData[i]) {
            // send only when data has changed
					  Serial.print("new data:");
					  Serial.println(recChanData[i]);
					  setLNTurnouts(firstTurnoutLNAddress[i], recChanData[i], 
					     oldRecChanData[i], startUp[i]);
					  oldRecChanData[i] = recChanData[i];
				  }
          startUp[i] = 0;  // reset startUp
			  }
        
      }  // end for(0..3)
      
		}
	}

	/* Check for any received loconet packets */
	LnPacket = LocoNet.receive();
	if (LnPacket) {

#ifdef DEBUG
		/* If a packet has been received, get the length of the
		 loconet packet */
		uint8_t LnPacketSize = getLnMsgSize(LnPacket);
		//Serial.print("Received a message of ");
		//Serial.print(LnPacketSize, DEC);
		//Serial.println(" Bytes from loconet,");
#endif

		LocoNet.processSwitchSensorMessage(LnPacket);
	}  // LnPacket

	// update SXnet at most every 200msecs (=UPDATE_INTERVAL)
	if ((millis() - lastUpdate) > UPDATE_INTERVAL) {
		lastUpdate = millis();
		for (uint8_t i = 0; i <= (MAX_SX_ADDR - MIN_SX_ADDR); i++) {
			if (sxChanged[i]) {

				uint8_t a = i + MIN_SX_ADDR;

				if (client.connected()) {
					sxChanged[i] = false;
					client.print("S ");
					client.print(a);
					client.print(" ");
					client.println(sxData[i]);
#ifdef DEBUG
					Serial.print("S ");
					Serial.print(a);
					Serial.print(" ");
					Serial.println(sxData[i]);
#endif					
				}
			} // sxChanged[i]
		} // i 0..N_ADDR
	}  // update-interval

  // check, if we need to request the turnout channels
  requestTurnoutStates();
  
	// check connection state and try to reconnect, if not connected
	if (!client.connected()) {
		Serial.println("disconnected.");
		client.stop();
		delay(1000);
		if ((millis() - lastRetry) > RETRY_INTERVAL) {
			lastRetry = millis();
			connectToSXnet();
		}
	}
}

// This call-back function is called from LocoNet.processSwitchSensorMessage
// for all Sensor messages
void notifySensor(uint16_t Address, uint8_t State) {
#ifdef DEBUG
	Serial.print("Sensor: ");
	Serial.print(Address, DEC);
	Serial.print(" - ");
	Serial.println(State ? "Active" : "Inactive");
#endif
	byte sxAddr = (Address - 1) / LN_MULTIPLIER;
	byte sxBit = (Address - 1) - LN_MULTIPLIER * sxAddr;  // sxBit 0 ..7
	if ((sxAddr >= MIN_SX_ADDR) && (sxAddr <= MAX_SX_ADDR)) {
		bitWrite(sxData[sxAddr - MIN_SX_ADDR], sxBit, State);
		sxChanged[sxAddr - MIN_SX_ADDR] = true;
	}
}

void setLNTurnout(int address, byte dir) {
	sendOPC_SW_REQ(address - 1, dir, 1);
	sendOPC_SW_REQ(address - 1, dir, 0);
}

void sendOPC_SW_REQ(int address, byte dir, byte on) {
	lnMsg SendPacket;

	int sw2 = 0x00;
	if (dir)
		sw2 |= B00100000;
	if (on)
		sw2 |= B00010000;
	sw2 |= (address >> 7) & 0x0F;

	SendPacket.data[0] = OPC_SW_REQ;
	SendPacket.data[1] = address & 0x7F;
	SendPacket.data[2] = sw2;
	//Serial.println("TX OPC_SW_REQ");
	LocoNet.send(&SendPacket);
}
