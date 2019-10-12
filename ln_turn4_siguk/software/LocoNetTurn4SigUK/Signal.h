/* 

Signal.h

 *  Created on: 2.10.2019
 *  Changed on:
 *  Version:
 *  Copyright:  Michael Blank
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#ifndef SIGNAL_H_
#define SIGNAL_H_
#include <WString.h>
#include <inttypes.h>

#define N_LEDS     4    // maximum 4 LEDs

class Signal {
   
public:
    Signal(uint8_t);  // 4 aspect signal
           // red, green, yellow, y+feather
    void process(void);  // needed for fade effects
    uint8_t act_state[N_LEDS];  // actual (analog) value of pin
    void init(void);
    void setRG(uint8_t value);  // set red or green (first ln-address)
    void setY(uint8_t value);   // set yellow (second ln-address)
    void setFeather(uint8_t value, uint8_t inverted);  // enable feather (will NOT be shown when RED is on)


private:
    void set(uint8_t value);   // 0 ... (N-1)
    void calcStateAndSet(void);  // calc internal state from all 3 input address/value pairs
    void output(void);   // set ports

    uint8_t number;   // signal A (=0) or B (=1)

    uint8_t fin_state[N_LEDS];
    uint8_t state;  // contains last value of command = last known signal state
    uint32_t mytimer = 0;

    uint8_t state_rg = 0;
    uint8_t state_y = 0;
    uint8_t state_feather = 0;
};

#endif /* SIGNAL_H_ */

