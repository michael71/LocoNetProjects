/* 

 signal_def.h


 *  Created on: 30 nov 2016
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


#ifndef SIGNAL_DEF_H_
#define SIGNAL_DEF_H_
#include <WString.h>
#include <inttypes.h>

// for english signals / Common Ground (A2982)
// value 0 = RED
// value 1 = GREEN
// value 2 = YELLOW
// value 3 = Feather & YELLOW

//for faster direct bitwise operations

#define SIG_A_R           A0   
#define SIG_A_R_DDR       DDRC      
#define SIG_A_R_PORT      PORTC           
#define SIG_A_R_PPIN      PORTC0  

#define SIG_A_G           12   
#define SIG_A_G_DDR       DDRB      
#define SIG_A_G_PORT      PORTB           
#define SIG_A_G_PPIN      PORTB4    

#define SIG_A_Y           11   
#define SIG_A_Y_DDR       DDRB      
#define SIG_A_Y_PORT      PORTB           
#define SIG_A_Y_PPIN      PORTB3   

#define SIG_A_4           10   
#define SIG_A_4_DDR       DDRB      
#define SIG_A_4_PORT      PORTB           
#define SIG_A_4_PPIN      PORTB2   

#define SIG_A_R_(val)   (bitWrite(SIG_A_R_PORT, SIG_A_R_PPIN, val)) 
#define SIG_A_G_(val)   (bitWrite(SIG_A_G_PORT, SIG_A_G_PPIN, val)) 
#define SIG_A_Y_(val)   (bitWrite(SIG_A_Y_PORT, SIG_A_Y_PPIN, val)) 
#define SIG_A_4_(val)   (bitWrite(SIG_A_4_PORT, SIG_A_4_PPIN, val)) 

#define SIG_B_R           9   
#define SIG_B_R_DDR       DDRB      
#define SIG_B_R_PORT      PORTB           
#define SIG_B_R_PPIN      PORTB1  

#define SIG_B_G           5
#define SIG_B_G_DDR       DDRD
#define SIG_B_G_PORT      PORTD
#define SIG_B_G_PPIN      PORTD5

#define SIG_B_Y           7   
#define SIG_B_Y_DDR       DDRD      
#define SIG_B_Y_PORT      PORTD           
#define SIG_B_Y_PPIN      PORTD7   

#define SIG_B_4           3
#define SIG_B_4_DDR       DDRD      
#define SIG_B_4_PORT      PORTD           
#define SIG_B_4_PPIN      PORTD3

#define SIG_B_R_(val)   (bitWrite(SIG_B_R_PORT, SIG_B_R_PPIN, val)) 
#define SIG_B_G_(val)   (bitWrite(SIG_B_G_PORT, SIG_B_G_PPIN, val)) 
#define SIG_B_Y_(val)   (bitWrite(SIG_B_Y_PORT, SIG_B_Y_PPIN, val)) 
#define SIG_B_4_(val)   (bitWrite(SIG_B_4_PORT, SIG_B_4_PPIN, val)) 

#endif /* SIGNAL_DEF_H_ */

