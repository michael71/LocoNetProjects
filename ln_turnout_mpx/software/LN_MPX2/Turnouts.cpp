/*

model 1..4 turnouts (not just a single one!)
 
*/


#include <Arduino.h> 
#include "Turnouts.h"

// create 4 turnouts
Turnouts::Turnouts(uint8_t t0, uint8_t t1, uint8_t t2, uint8_t t3) {
   // pins for output
   _pin[0] = t0;
   _pin[1] = t1;
   _pin[2] = t2;
   _pin[3] = t3;
   _enable = 0;
}

// initialize all turnouts to "CLOSED"
void Turnouts::init(uint8_t en) {
  _enable = en;
  for (int i =0; i < N_TURNOUTS; i++) {
     pinMode(_pin[i], OUTPUT);
     _value[i]= TURNOUT_CLOSED;
     digitalWrite(_pin[i], _value[i]);
     _state[i]= TURNOUT_CLOSED;
  }
  pinMode(en, OUTPUT);
  digitalWrite(en, HIGH);  //enable the LM1909's
}

// return current state of turnout n
uint8_t Turnouts::set(uint8_t n, uint8_t v) {
  if  ((n < N_TURNOUTS) // n is always >=0
      &&
      ((v == TURNOUT_CLOSED) || (v == TURNOUT_THROWN))
      ) {
      _value[n] = v;
      digitalWrite(_pin[n], _value[n]);
      _state[n] = _value[n];
      pinMode(_enable, OUTPUT);
      digitalWrite(_enable, HIGH);
      return _state[n];  // executed.
  } else {
      // cannot interpret command
      return INVALID;
  }
}

uint8_t Turnouts::get(uint8_t n) {
	if (n < N_TURNOUTS) {
		return _state[n];
	} else {
		return INVALID;
	}
}

