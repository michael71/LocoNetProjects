/*
 * mpx_signal.h
 *
 *  Created on: 23.04.2017
 *      Author: mblank
 */

#ifndef MPX_SIGNAL_H_
#define MPX_SIGNAL_H_

#include <Arduino.h>

const uint8_t sigtype[2] = {6 , 6};   // NOT USED SO FAR


#define MMAX  8    //Anzahl Signalbilder

// Signalbilder ph=0    /  ph=1
// 0 = Hp0    = - x + x / x x x x
// 1 = Ks1    = + x - x / x x x x
// 2 = Ks2    = x + - x / x x x x
// 3 =Ks1+Zs3 = + x - x / + x x -
// 4 =Hp0+sh1 = - + + x / + - x x
// b == -/blinkend

const char sigmap[MMAX][9] = {     // len=9 for terminating \0
    "-x+xxxxx",  // hp0       signalbild 0
    "+x-xxxxx",  // Ks1       signalbild 1
    "x+-xxxxx",  // Ks2       .... usw
    "+x-x+xx-",  //Ks1+Zs3
    "-++x+-xx",  //Hp0+sh1
    "x-+xxxxx",  // WS1
    "+xbx-xx+",  //Ks1+Zs3v
    "-x+x+bxx",  // hp0+ZS1
};

const char sigbild[MMAX][9] = {    // Name der Signalbilder
   "Hp0",
   "Ks1",
   "Ks2",
   "Ks1+Zs3",
   "Hp0+sh1",
   "W1",
   "Ks1+Zs3v",
   "Hp0+Zs1"
};

#define NTYPE  7   // Anzahl signaltypen
/* const uint8_t allowed[NTYPE][MMAX] = {
  { 0, 0, 0, 0, 0, 0, 0, 0},  // Viessamnn 4040 (Vorsignal)
  { 0, 0, 0, 0, 0, 0, 0, 0},  // 4041 gibt es nicht
  { 0, 0, 0, 0, 0, 0, 0, 0},  // 4042 noch nicht implementiert
  { 1, 1, 0, 1, 1, 0, 0, 1},  // 4043 KS Hauptsignal als Ausfahrsignal
  { 0, 0, 0, 0, 0, 0 ,0, 0},  // 4044 gibt es nicht
  { 1, 1, 1, 1, 0, 1, 1, 0},  // 4045 Einfahrsignal
  { 0, 0, 0, 0, 0, 0, 0, 0}   // 4046 noch nicht implementiert
}; */

// table for dimming signals  start -> ......      0=change val .....    ->end
const uint8_t dimtable[16] = { 8, 4, 2, 2, 1, 1, 1, 0, 1, 1, 1, 2, 2, 4, 8, 8 };

class MPX_Signal {

public:
    MPX_Signal(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4);
           // red, green, (green+yellow), white (sh0)
    void process(void);  // needed for displaying two pairs of leds at the same time

    void setln(uint8_t n, uint8_t val);   // set n-th lnval bit)

    void init(void);
    uint8_t getln(void);

    uint8_t state;  // contains last value of command = last known signal state
    uint32_t dimTimer;
    int8_t dimCount, dim;
    uint8_t lnval;  // bit values of all LN channels used (max=8)
 
private:
    uint8_t set(uint8_t value);   // 0 ... MMAX
    void setPhase(uint8_t ph, uint8_t dimval);  // ph=0 or ph=1
    void switchOff(void);
    uint8_t get();   // return current value
    uint8_t value, newvalue;
    uint8_t count;
    bool startdim;
    uint8_t sigpin[4];  // each mpx signal uses 4 Arduino PINs

};

#endif /* MPX_SIGNAL_H_ */
