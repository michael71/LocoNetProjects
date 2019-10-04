/*
 * config.h
 *
 *  Created on: 2.10.2019
 *      Author: mblank
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// article number: 51120 - only first 4 digits used
#define ARTNR   5112     // this is fixed and assigned to Turn4SigUK HW
#define LNCV_COUNT (16)
#define LNCV_MAX_USED (12)
#define LNCV_DEBUG (LNCV_COUNT - 1)

/* store 16 LNCVs:
  0  module number (to be able to distinguish between modules with identical art.no.)
  1  turnout 1 address
  2  turnout 2 address
  3  turnout 3 address
  4  turnout 4 address
  5  signal 1 address
  6  signal 1 sec.address
  7  signal 1 feather address
  8  signal 1 feather inverted flag (if != 0)
  9  signal 2 address 
  10 signal 2 sec.address
  11 signal 2 feather address
  12 signal 2 feather inverted flag (if != 0)
  13 tbd
  14 tbd
  15 debug output on   (if other than 0 or 65535)
  */




#endif /* CONFIG_H_ */
