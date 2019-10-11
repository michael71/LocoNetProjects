/*

 model a SINGLE signal with fading leds
 
 version for english signals, Common Ground (A2982)

 */

#include <Arduino.h> 
#include "signal_def.h"
#include "Signal.h"

// create a signal with 4 aspects
Signal::Signal(uint8_t n) {
	number = n;   // A=0, B=1
	mytimer = 0;
}

void Signal::init() {
	for (uint8_t i = 0; i < N_ASPECTS; i++) {
		fin_state[i] = act_state[i] = 0;  // off
	}

	state_rg = 0;  // => red
	state_y = 0;   // => yellow off
	state_f = 0;   // => feather off
    calcStateAndSet();

	if (number == 0) {
		pinMode(SIG_A_R, OUTPUT);
		pinMode(SIG_A_G, OUTPUT);
		pinMode(SIG_A_Y, OUTPUT);
		pinMode(SIG_A_4, OUTPUT);
	} else {
		pinMode(SIG_B_R, OUTPUT);
		pinMode(SIG_B_G, OUTPUT);
		pinMode(SIG_B_Y, OUTPUT);
		pinMode(SIG_B_4, OUTPUT);
	}

}

void Signal::calcStateAndSet() {
	if (state_rg == 0) {
		// red
		state = 0;
	} else if (state_y == 0) {
		// green or green with feather
		if (state_f == 0) {
			state = 1;
		} else {
			state = 4;
		}
	} else {
		// yellow or yellow with feather
		if (state_f == 0) {
			state = 2;
		} else {
			state = 3;
		}
	}
	set(state);
	Serial.print("state=");
	Serial.println(state);
}

void Signal::setRG(uint8_t value) {
	state_rg = value;  // set red or green (first ln-address)
	calcStateAndSet();
}
void Signal::setY(uint8_t value) {
	state_y = value; // set yellow (second ln-address)
	calcStateAndSet();
}
void Signal::setFeather(uint8_t value) {
	// enable feather (will NOT be shown when RED is on)
    Serial.print("feath=");
    Serial.println(value);
	if (value) {
	    state_f = 1;
	} else {
		state_f = 0;
	}
	calcStateAndSet();
}


// version for english signals, Command Ground (A2982)
// value 0 = RED
// value 1 = GREEN
// value 2 = YELLOW
// value 3 = Feather & YELLOW
// value 4 = Feather & GREEN

void Signal::set(uint8_t value) {
	switch (value) {
	case 0:
		fin_state[0] = 16;
		fin_state[1] = 0;
		fin_state[2] = 0;
		fin_state[3] = 0;
		state = 0;
		break;
	case 1:
		fin_state[0] = 0;
		fin_state[1] = 16;
		fin_state[2] = 0;
		fin_state[3] = 0;
		state = 1;
		break;
	case 2:
		fin_state[0] = 0;
		fin_state[1] = 0;
		fin_state[2] = 16;
		fin_state[3] = 0;
		state = 2;
		break;
	case 3:
		fin_state[0] = 0;
		fin_state[1] = 0;
		fin_state[2] = 16;
		fin_state[3] = 16;
		state = 3;
		break;
    case 4:
		fin_state[0] = 0;
		fin_state[1] = 16;
		fin_state[2] = 0;
		fin_state[3] = 16;
		state = 4;
		break;
	//default:
		// do nothing
	}
	output();
}

void Signal::output(void) {

	if (number == 0) {
		if (fin_state[0]) SIG_A_R_(1);
		else SIG_A_R_(0);
		if (fin_state[1]) SIG_A_G_(1);
		else SIG_A_G_(0);
		if (fin_state[2]) SIG_A_Y_(1);
		else SIG_A_Y_(0);
		if (fin_state[3]) SIG_A_4_(1);
		else SIG_A_4_(0);
	} else if (number == 1) {
		if (fin_state[0]) SIG_B_R_(1);
		else SIG_B_R_(0);
		if (fin_state[1]) SIG_B_G_(1);
		else SIG_B_G_(0);
		if (fin_state[2]) SIG_B_Y_(1);
		else SIG_B_Y_(0);
		if (fin_state[3]) SIG_B_4_(1);
		else SIG_B_4_(0);
	}
}

/*
void Signal::process(void) {
	
	if ((millis() - mytimer) < 25)
		return;

	mytimer = millis();
	// check for dimming LEDs (up/down), analog value range 0..255
	for (uint8_t i = 0; i < N_ASPECTS; i++) {
		if (act_state[i] < fin_state[i]) {
			// increment 
			uint8_t intens = act_state[i] + 2;
			if (intens > 16) {
				intens = 16;
			}
			act_state[i] = intens;
			// value gets written in interrupt routine
		}
		if (act_state[i] > fin_state[i]) {
			// decrement
			uint8_t intens = act_state[i] - 2;
			if (intens < 0) {
				intens = 0;
			}
			act_state[i] = intens;
			// value gets written in interrupt routine
		}
	}
} */

