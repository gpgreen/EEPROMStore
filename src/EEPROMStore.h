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
// values with the hi bit reserved, the maximum mileage stored is 0x8fff
// or 36863. To allow the mileage to accumulate more than this, we use
// a byte in the header, to store how many iterations of the max
// number are used. If the header byte is 0, then the mileage is the
// value stored, if it is 1, then add 36863 to the value stored, and
// so on. This allows for a maximum mileage of 9,436,928. Once this
// value is reached, it will roll over and start from 0 again.
//
// To write a new mileage, write the new value at the current
// write offset, then write the marker byte (hi bit set) following.
//
// See scanEEPROMForLatest for the initiation of this algorithm

const int METRIC_FLAG = 0x1;

// define if you want to see debug messages on the serial port
#define SERIAL_DEBUG_MSG

struct TripMarker
{
    byte multiplier;
    word marker;
};

struct EEPROMHeader 
{
    byte version;
    byte flags;
    word rpm_range;
    byte contrast;
    byte multiplier;
    byte backlight_hi;
    byte backlight_lo;
    float voltage_offset;
    float voltage_correction;
    float speedo_correction;
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
    unsigned long mileage();

    // add to the current mileage
    void addMileage(unsigned long val);

    // set mileage
    void setMileage(unsigned long val);
    
    // get rpm range
    word rpmRange();
    void setRPMRange(word range);

    // get contrast
    uint8_t contrast();
    void setContrast(uint8_t newval);
    
    // get backlight
    int backlight();
    void setBacklight(int newval);

    // get voltage offset
    float voltageOffset();
    void setVoltageOffset(float newval);

    // get voltage correction
    float voltageCorrection();
    void setVoltageCorrection(float newval);

    // get speedo correction
    float speedoCorrection();
    void setSpeedoCorrection(float newval);

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
    unsigned long trip1();
    unsigned long trip2();
    
private:

    // read the header field from the EEPROM
    // this contains rarely written values
    void readEEPROMHeader();

    // find the current mileage stored in EEPROM
    void readMileage();

    // write a new mileage value, updates the multiplier if need be
    void writeLatestEEPROM(word val);

    // read the EEPROM value array to get the latest mileage value
    void scanEEPROMForLatest();

    // set the header structure to default values
    void resetHeader();

    // update values in the header to EEPROM
    void updateHeader();

    // manipulate mileage and multiplier pairs
    unsigned long multiplyMileage(byte multiplier, word val);
    bool collapseMileage(unsigned long mileage, byte& multiplier, word& val);

    // the header
    struct EEPROMHeader _header __attribute__ ((aligned (4)));
	
    // offset in eeprom to latest mileage value
    int _latest_offset;

    // latest mileage value in eeprom (not real mileage, due to multiplier)
    word _latest_val;

    // current mileage
    unsigned long _mileage;

    // last mileage written to eeprom
    unsigned long _written_mileage;
};

#endif /* EEPROMSTORE_H_ */
