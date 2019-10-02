/*

 model 1..4 turnouts (not just a single one!)
 
 */

#include <Arduino.h> 
#include "Turnouts.h"

// create 1 turnouts
Turnouts::Turnouts(uint8_t t0) {
	// pins for output
	_pin[0] = t0;
	_npins = 1;
}

// create 2 turnouts
Turnouts::Turnouts(uint8_t t0, uint8_t t1) {
	// pins for output
	_pin[0] = t0;
	_pin[1] = t1;
	_npins = 2;
}

// create 3 turnouts
Turnouts::Turnouts(uint8_t t0, uint8_t t1, uint8_t t2) {
	// pins for output
	_pin[0] = t0;
	_pin[1] = t1;
	_pin[2] = t2;
	_npins = 3;

}

// create 4 turnouts
Turnouts::Turnouts(uint8_t t0, uint8_t t1, uint8_t t2, uint8_t t3) {
	// pins for output
	_pin[0] = t0;
	_pin[1] = t1;
	_pin[2] = t2;
	_pin[3] = t3;
	_npins = 4;
}

// create 8 turnouts
Turnouts::Turnouts(uint8_t t0, uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4,
		uint8_t t5, uint8_t t6, uint8_t t7) {
	// pins for output
	_pin[0] = t0;
	_pin[1] = t1;
	_pin[2] = t2;
	_pin[3] = t3;
	_pin[4] = t4;
	_pin[5] = t5;
	_pin[6] = t6;
	_pin[7] = t7;
	_npins = 8;
}

// initialize all turnouts to "CLOSED"
void Turnouts::init(uint8_t en) {
	for (int i = 0; i < _npins; i++) {
		pinMode(_pin[i], OUTPUT);
		_value[i] = CLOSED;
		digitalWrite(_pin[i], _value[i]);
		_state[i] = CLOSED;
	}
	pinMode(en, OUTPUT);
	digitalWrite(en, HIGH);  //enable the LM1909's
}

// return current state of turnout n
uint8_t Turnouts::set(uint8_t n, uint8_t v) {
	if ((n < _npins) // n is always >=0
	&& ((v == CLOSED) || (v == THROWN))) {
		_value[n] = v;
		digitalWrite(_pin[n], _value[n]);
		_state[n] = _value[n];
		return _state[n];  // executed.
	} else {
		// cannot interpret command
		return INVALID;
	}
}

uint8_t Turnouts::get(uint8_t n) {
	return _state[n];
}

