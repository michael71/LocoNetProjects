# LocoNetProjects - LN to Selectrix Bridge

## Operating Principle

To use LN-hardware on our Selectrix based club layout a "bridge" was built which currently 

1) receives the LocoNet "sensor" messages and 
2) sends them to "sxnet" (TCP messages) which are interpreted by the "SX3-PC" program and 
allow to map LN-sensor-states to SX-sensor-states.
3) allways 8 LN-addresses like 801 ... 808 are mapped to ONE sx-address (80) with its 
8 bit of data (bit1= 801, ... bit8= 808)

##Hardware:
Arduino-Ethernet
Fremo LN-Shield (see http://nh-finescale.nl/fremo/dcc/fremo-ln-shield/FremoLNShield.html )

Jumper Settings LN-Shield:

Version 1: pure interface, using an Intellibox or similiar for control

Function 	Jumper 	Description
Loconet Pullup 	P_UP 	OFF
Railsync 	RS 	OFF
Loconet 	TX 	T6/T7 	T6
Loconet 	RX 	RX 	ON
VIN 		JP5 	OFF
Power 		J3 	OFF

Version 2: The LN-Shield is powering the LN-Bus (no central command station !)

Function 	Jumper 	Description
Loconet Pullup 	P_UP 	ON      
Railsync 	RS 	ON      => LN-Shield powers LocoNet
Loconet 	TX 	T6/T7 	T6
Loconet 	RX 	RX 	ON
VIN 		JP5 	ON
Power 		J3 	12v power supply



##Software:
Arduino SW "LocoNetSensorsToSXNet", based on LocoNet library,see https://github.com/mrrwa 


BETA !


Model Railroad Arduino Projects based on LocoNet and MRRWA code (see https://github.com/mrrwa )

Hardware is "OpenHardware" and in some cases is ported from https://opensx.net projects.
