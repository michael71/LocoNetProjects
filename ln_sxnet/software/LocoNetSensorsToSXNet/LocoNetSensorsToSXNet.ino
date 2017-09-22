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
 * Title :   LocoNetSensorsToSXNet
 * Author:   Michael Blank
 * Date:     09-Sep-2017
 * Software: AVR-GCC
 * Target:   Arduino
 * 
 * 
 * DESCRIPTION
 *   sends all sensor messages (received from LocoNet) in an address range from
 *   851 to 1041 to SXnet 85 to 104 
 *   channel 85, sxbis1 = 851 ...
 *   channel 85, sxbit8 = 858 etc 
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

EthernetClient client;
//IPAddress server(192, 168, 178, 29);
IPAddress server(192, 168, 1, 30);

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

	/* Configure the serial port for 57600 baud */
	Serial.begin(57600);
	Serial.println(F("LocoNet Sensors -> SXnet started!!"));
	Serial.print(F("loconet sensor addresses must be in range "));
	Serial.print(MIN_SX_ADDR * LN_MULTIPLIER + 1 );
	Serial.print(" to ");
	Serial.println(MAX_SX_ADDR * LN_MULTIPLIER + 1);
	Serial.print(F("Server address="));
	String sIP = IpAddress2String(server);
	Serial.println(sIP);

	/*Initialize a LocoNet packet buffer to buffer bytes from the PC */
	initLnBuf(&LnTxBuffer);

	/* initialise the ethernet device */
	//Ethernet.begin(mac, ip, gateway, subnet);
  Ethernet.begin(mac);

  Serial.print("own ip=");
  Serial.println(Ethernet.localIP());
  
	/* connect */
	connectToSXnet();

}

void loop() {

	if (client.available()) {
		char c = client.read();
		Serial.print(c);
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
	byte sxAddr = (Address -1) / LN_MULTIPLIER;
	byte sxBit = (Address -1) - LN_MULTIPLIER * sxAddr;  // sxBit 0 ..7
	if ((sxAddr >= MIN_SX_ADDR) && (sxAddr <= MAX_SX_ADDR)) {
		bitWrite(sxData[sxAddr - MIN_SX_ADDR], sxBit, State);
		sxChanged[sxAddr - MIN_SX_ADDR] = true;
	}
}

