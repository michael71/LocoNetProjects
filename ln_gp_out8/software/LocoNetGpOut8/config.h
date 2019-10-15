/*
 * config.h
 *
 *  Created on: 2.10.2019
 *      Author: mblank
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// article number: 51130 - only first 4 digits used
#define ARTNR   5113    // this is fixed and assigned to GP-Out8 HW
#define LNCV_COUNT (16)
#define LNCV_MAX_USED (8)
#define LNCV_DEBUG (LNCV_COUNT - 1)

/* store 16 LNCVs:
  0  module number (to be able to distinguish between modules with identical art.no.)
  1  out1 address
  2  out2 address
  3  out3 address
  4  out4 address
  5  out5 address
  6  out6 address
  7  out7 address
  8  out8 address
  9  tbd
  10 tbd 
  11 tbd 
  12 tbd 
  13 tbd
  14 tbd
  15 debug output on   (if other than 0 or 65535)
  */




#endif /* CONFIG_H_ */
