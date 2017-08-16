//============================================================================
// Name        : EEPROMStore.h
// Author      : Greg Green <gpgreen@gmail.com>
// Version     : 1.0
// Copyright   : GPL v3
// Description : EEPROM Store for motorcycle gauge
//============================================================================

#ifndef EEPROMSTORE_H_
#define EEPROMSTORE_H_

#include <EEPROM.h>

// To save wear and tear on the eeprom, write mileage values to the eeprom
// in sequence. At the starting offset, write 2 byte pairs which contain
// the mileage value. The high bit of the value is reserved to indicate the
// latest value of mileage. When reading the sequence of values, if the high
// bit changes from one value to the next, the previous value is the latest
// mileage value. Since we are writing 2 byte values with the hi bit reserved,
// the maximum mileage stored is 2^15 or 32768. To allow the mileage to accumulate
// more than this, we use the 3rd byte in the header, to store how many iterations
// of the 2^15 number are used. If the 3rd byte is 0, then the mileage is the value
// stored, if it is 1, then add 32768 to the value stored, and so on. This allows
// for a maximum mileage of 8,388,607. Once this value is reached, it will roll over
// and start from 0 again
//
// To write a new mileage, set the high bit of the value to the flag value, then
// write the 2 bytes in sequence. The offset is incremented to point to the next
// value position. If the end of the eeprom array is reached, the flag bit value
// is flipped. This way the flag flipping to find the latest value is only good
// while scanning the squenece, ie if we hit the end, the last value is the latest.
// The next write will be at the beginning with the opposite flag

struct EEPROMHeader 
{
  byte version;
  byte hi_byte_rpm_range;
  byte lo_byte_rpm_range;
  byte contrast;
  byte multiplier;
  byte backlight_hi;
  byte backlight_lo;
};

class EEPROMStore
{
public:
	explicit EEPROMStore();

	// read the eeprom and initialize all state
	void begin();
	
	// initialize the eeprom to it's starting state with zero mileage
	void initializeEEPROM();

	// write the current mileage in the EEPROM, no effect
	// if the value is the same as already stored
	void writeMileage();

	// get the current mileage
	long mileage();

	// add to the current mileage
	void addMileage(long val);

	// get rpm range
	long rpmRange();

	// get contrast
	uint8_t contrast();

	// get backlight
	int backlight();
	
private:

	// read the header field from the EEPROM
	// this contains rarely written values
	void readEEPROMHeader();

	// find the current mileage stored in EEPROM
	void readMileage();

	// write a new mileage value, updates the multiplier if need be
	void writeLatestEEPROM(long val);

	// read the EEPROM value array to get the latest mileage value
	void scanEEPROMForLatest();

	// set the header structure to default values
	void resetHeader();
	
	// the header
	struct EEPROMHeader _header;
	
	// offset in eeprom to latest mileage value
	int _latest_offset;

	// latest mileage value in eeprom (not real mileage, due to multiplier)
	long _latest_val;

	// flag to set in high bit of mileage value in eeprom
	byte _hi_bit;

	// current mileage
	long _mileage;

	// last mileage written to eeprom
	long _written_mileage;
};

#endif /* EEPROMSTORE_H_ */
