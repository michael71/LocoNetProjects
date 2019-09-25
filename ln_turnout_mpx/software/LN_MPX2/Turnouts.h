/* 

Signal.h

 *  Created on: 07,03.2016
 *  Changed on: 
 *  Version:    0.1
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


#ifndef TURNOUTS_H_
#define TURNOUTS_H_
#include <WString.h>
#include <inttypes.h>

#define TURNOUT_CLOSED      (0)
#define TURNOUT_THROWN      (1)

#define N_TURNOUTS   (4)   // always 4 turnouts
#define INVALID      (99)    // invalid value


class Turnouts{
   
public:

    Turnouts(uint8_t, uint8_t, uint8_t, uint8_t);  // 4 turnouts
    uint8_t set(uint8_t n, uint8_t value);   // n=0 ... N turnouts
    uint8_t get(uint8_t n);   // 
    void init(uint8_t); // init and enable LB1909MC's


private:
    uint8_t _value[N_TURNOUTS];  // actual value of pin
    uint8_t _pin[N_TURNOUTS];  // arduino pin number 
    uint8_t _state[N_TURNOUTS];  // current state of turnout
    uint8_t _enable;   // enable LB1909MC pin
};

#endif /* SIGNAL_H_ */

