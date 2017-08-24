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

// To save wear and tear on the eeprom, write mileage values to the
// eeprom in sequence. At the starting offset, write 2 byte pairs
// which contain the mileage value. After the latest mileage, write
// one byte with the high bit set to indicate end of updates.  When
// reading the sequence of values, as soon as the marker byte is read,
// the last value is the current mileage Since we are writing 2 byte
// values with the hi bit reserved, the maximum mileage stored is 2^15
// or 32768. To allow the mileage to accumulate more than this, we use
// a byte in the header, to store how many iterations of the 2^15
// number are used. If the header byte is 0, then the mileage is the
// value stored, if it is 1, then add 32768 to the value stored, and
// so on. This allows for a maximum mileage of 8,388,607. Once this
// value is reached, it will roll over and start from 0 again.
//
// To write a new mileage, write the new value at the current
// write offset, then write the marker byte (hi bit set) following.
//
// See scanEEPROMForLatest for the initiation of this algorithm

const int METRIC_FLAG = 0x1;

struct TripMarker
{
    byte multiplier;
    long marker;
};

struct EEPROMHeader 
{
    byte version;
    byte flags;
    long rpm_range;
    byte contrast;
    byte multiplier;
    byte backlight_hi;
    byte backlight_lo;
    float voltage_correction;
    TripMarker trip1;
    TripMarker trip2;
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
    int mileage();

    // add to the current mileage
    void addMileage(int val);

    // set mileage
    void setMileage(int val);
    
    // get rpm range
    long rpmRange();
    void setRPMRange(long range);

    // get contrast
    uint8_t contrast();
    void setContrast(uint8_t newval);
    
    // get backlight
    int backlight();
    void setBacklight(int newval);

    // get voltage correction
    float voltageCorrection();
    void setVoltageCorrection(float newval);

    // test for units
    bool isMetric();
    bool isImperial();

    // set units, writes the EEPROM
    void setMetric();
    void setImperial();

    // resett trip markers - set marker to current mileage settings
    void resetTrip1();
    void resetTrip2();

    // get current trip value
    int trip1();
    int trip2();
    
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

    // update values in the header to EEPROM
    void updateHeader();

    // manipulate mileage and multiplier pairs
    int multiplyMileage(byte multiplier, long val);
    bool collapseMileage(int mileage, byte& multiplier, long& val);

    // the header
    struct EEPROMHeader _header __attribute__ ((aligned (4)));
	
    // offset in eeprom to latest mileage value
    int _latest_offset;

    // latest mileage value in eeprom (not real mileage, due to multiplier)
    long _latest_val;

    // current mileage
    int _mileage;

    // last mileage written to eeprom
    int _written_mileage;
};

#endif /* EEPROMSTORE_H_ */
