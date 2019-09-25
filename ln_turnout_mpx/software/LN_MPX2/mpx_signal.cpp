/*
 * mpx_signal.cpp
 *
 *  Created on: 23.04.2017
 *      Author: mblank
 */

#include <Arduino.h>

#include "mpx_signal.h"

#define SCOPE  (2)   // pin 2 = scope timing measurement

/* const uint8_t sigpin[2][4] = {
  { A0, 12, 11, 10},
  { 9, 5, 7, 3}
}; */

// create mpx signal
MPX_Signal::MPX_Signal(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4) {
	sigpin[0] = p1;
	sigpin[1] = p2;
	sigpin[2] = p3;
	sigpin[3] = p4;
	state = 0;
	count = 0;
	value = 0;  // Hp0
	newvalue = 0;
	lnval = 0;
	dimCount = 0;
	dimTimer = 0;
	dim = 0;
	startdim=false;
}

/**
 * set a value of an ln-channel as a bit in "lnval"
 * map to a state of the signal
 */
void MPX_Signal::setln(uint8_t n, uint8_t d) {
	if (n >= 8) return;

	uint8_t b = 0x01 << n;
	if (d) {
		lnval |= b;   // set bit n
	} else {
		lnval &= ~b;  // clear bit n
	}

	// mapping of ln-values to signal states (to be changed to indexing)
	switch (lnval) {
	case 0:
		set(0);   //hp0
		Serial.println("0 set(0) Signal hp0");
		break;
	case 1:
		set(3);   //KS1+zs3
		Serial.println("1 set(3) Signal Ks1+Zs3");
		break;
	case 2:
		set(4);   // sh1
		Serial.println("2 set(2) Signal Sh1");
		break;
	case 3:
		//set(3);   //KS1+zs3
		Serial.println("3 ??? Signal ???");
		break;
	case 4:
		set(0);   //KS1+zs3
		Serial.println("4 set(0) Signal hp0");
		break;
	case 5:
		set(3);   //KS1+zs3
		Serial.println("5 set(3) Signal Ks1+Zs3");
		break;
	case 6:
		//set(3);   //KS1+zs3
		Serial.println("6 ??? Signal ???");
		break;
	case 7:
		//set(3);   //KS1+zs3
		Serial.println("7 ??? Signal ???");
		break;
	}
}

uint8_t MPX_Signal::getln() {
	return lnval;
}


uint8_t MPX_Signal::set(uint8_t v) {
	if (v >= MMAX) return 0;

	if (value != v) {
		// new signal aspect
		startdim = true;
		dimTimer = millis();
		dim = 0;
		newvalue = v;
	}

	return 1;
}

uint8_t MPX_Signal::get() {
	return value;
}

void MPX_Signal::init() {
	set(0);   // Hp0 = default value
#ifdef SCOPE
	pinMode(SCOPE,OUTPUT);
#endif
}

/** ph => 0 or 1, dim = dimming state 0=off, 8=max
 *
 */
void MPX_Signal::setPhase(uint8_t ph, uint8_t dim) {
	uint8_t index, i, bli;
	bli = (uint8_t)((millis() >> 10) & 0x01);  // ~1000msec blink timer

	if (ph == 0) {  // increment dimCount only in one phase
		// (but use in both!)
		dimCount++; // dimcount 0 ... 7
		if (dimCount > 8) dimCount=0;
	}

	// switch leds off if dimCount greater than "dim" input value
	if (dimCount > dim) {
		switchOff();
		return;
	}

	for (i = 0; i < 4; i++) {  // outputs 0 .. 3 (for each signal)
		index = i +4*ph;       // index is phase dependent

		// for a given signal 'value' (=aspect),
		// lookup char-value from sigmap table to set DIR and OUTPUT

		switch (sigmap[value][index]) {
		case 'x':  // no output
			pinMode(sigpin[i],INPUT);
			break;
		case '-':  // - output
			pinMode(sigpin[i],OUTPUT);
			digitalWrite(sigpin[i],LOW);
			break;
		case '+':  // + plus output
			pinMode(sigpin[i],OUTPUT);
			digitalWrite(sigpin[i],HIGH);
			break;
		case 'b':  // alternate
			if (bli == 0) {
				pinMode(sigpin[i],OUTPUT);
			} else {
				pinMode(sigpin[i],INPUT);
			}
			digitalWrite(sigpin[i],LOW);
			break;
		default:  // should not happen
			Serial.println("ERROR");
		}

	}

}

void MPX_Signal::switchOff() {
	for (uint8_t i = 0; i < 4; i++)  {
		pinMode(sigpin[i],INPUT);
	}
}



void MPX_Signal::process(void) {

	if (startdim) {
		if ( (millis() - dimTimer) > 20) {
			dim++;
			dimTimer = millis();
			if (dim == 7) {
				value = newvalue;
			} else if (dim >= 15) {
				dim=0;
				startdim=false;
			}
		}
	}

	if (count >=2) count = 0;
	setPhase(count, dimtable[dim]);
	count++;

}




