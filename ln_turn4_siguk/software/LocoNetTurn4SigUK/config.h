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
#define LNCV_COUNT 12
/* store 12 LNCVs:
  0  module number (to be able to distinguish between modules with identical art.no.)
  1  turnout 1 address
  2  turnout 2 address
  3  turnout 3 address
  4  turnout 4 address
  5  signal 1 address (multiaspect via address+1)
  6  signal 1 feather address
  7  signal 1 feather inverted flag (if != 0)
  8  signal 2 address (multiaspect via address+1)
  9  signal 2 feather address
  10 signal 2 feather inverted flag (if != 0)
  11 debug output on   (if other than 0 or 65535)
  */
#define LNCV_DEBUG  (11)



#endif /* CONFIG_H_ */
