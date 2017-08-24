//============================================================================
// Name        : EEPROMStore.cpp
// Author      : Greg Green <gpgreen@gmail.com>
// Version     : 1.0
// Copyright   : GPL v3
// Description : EEPROM Store for motorcycle gauge
//============================================================================

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <stdlib.h>

#include "EEPROMStore.h"

// offset to beginning of eeprom mileage value array
int k_start_eeprom_array = sizeof(struct EEPROMHeader);

// eeprom library not working after length of 1024 bytes, even though the teensy 3.0 has 2048
int k_end_of_eeprom = 1024;

// The constructor
EEPROMStore::EEPROMStore()
    : _latest_offset(0), _latest_val(0), _mileage(0), _written_mileage(0)
{
    resetHeader();
}

void EEPROMStore::begin()
{
    //initializeEEPROM();
    readEEPROMHeader();
    readMileage();
}

void EEPROMStore::resetHeader()
{
    _header.version = 0;
    _header.flags = 0;
    _header.rpm_range = 12000L;
    _header.contrast = 50;
    _header.multiplier = 0;
    _header.backlight_hi = 0;
    _header.backlight_lo = 128;
    _header.voltage_correction = 1.0;
    _header.trip1.multiplier = 0;
    _header.trip1.marker = 0;
    _header.trip2.multiplier = 0;
    _header.trip2.marker = 0;
}

// read the header field from the EEPROM
// this contains rarely written values
void EEPROMStore::readEEPROMHeader() {
    Serial.print("EEPROM size:");
    Serial.println(EEPROM.length(), DEC);

    EEPROM.get(0, _header);

    Serial.print("EEPROM Header version:");
    Serial.println(_header.version, DEC);
    if (_header.version != 0) {
	initializeEEPROM();
	Serial.println("Reinitialized EEPROM as it was formatted incorrectly");
    }

    Serial.print("EEPROM Header [flags:0x");
    Serial.print(_header.flags, HEX);
    
    Serial.print(" rpm range:");
    Serial.print(_header.rpm_range, DEC);

    Serial.print(" contrast:");
    Serial.print(_header.contrast, DEC);

    Serial.print(" multiplier:");
    Serial.print(_header.multiplier, DEC);

    Serial.print(" backlight:");
    Serial.print((_header.backlight_hi << 8) + _header.backlight_lo, DEC);

    Serial.print(" volt corr:");
    Serial.print(_header.voltage_correction, 6);
    Serial.println("]");

    Serial.print("trip1 multiplier:");
    Serial.print(_header.trip1.multiplier, DEC);
    Serial.print(" marker:");
    Serial.println(_header.trip1.marker, DEC);

    Serial.print("trip2 multiplier:");
    Serial.print(_header.trip2.multiplier, DEC);
    Serial.print(" marker:");
    Serial.println(_header.trip2.marker, DEC);
}

// write the header with updated values
void EEPROMStore::updateHeader() {
    Serial.println("updateHeader");
    EEPROM.put(0, _header);
}

// initialize the eeprom to it's starting state with zero mileage
void EEPROMStore::initializeEEPROM() {
    Serial.println("initializeEEPROM");
    resetHeader();
    updateHeader();
    for (int i=k_start_eeprom_array; i<k_end_of_eeprom; ++i)
	EEPROM.write(i, 0);
    _mileage = _written_mileage = 0;
}

// read the EEPROM value array to get the latest mileage value
void EEPROMStore::scanEEPROMForLatest() {
    byte b;
    _latest_offset = k_start_eeprom_array;
    Serial.print("Scan eeprom for end marker");
    Serial.print(" at offset:");
    Serial.println(_latest_offset, DEC);
    for (int i=_latest_offset; i<k_end_of_eeprom; ++i) {
	b = EEPROM.read(i);
	Serial.print("b:");
	Serial.print(b, HEX);
	Serial.print(" offset:");
	Serial.print(i, DEC);
	if ((b & 0x80) == 0) {
	    _latest_val = (b << 8) + EEPROM.read(++i);
	    Serial.print(" val:");
	    Serial.println(_latest_val, DEC);
	} else {
	    break;
	}
	_latest_offset += 2;
    }
    // special case is EEPROM all 0, no values written yet
    if (_latest_offset >= k_end_of_eeprom) {
	_latest_offset = k_start_eeprom_array;
	_latest_val = 0;
	Serial.println("\nblank mileage");
    }
    Serial.print("\nwrite offset:");
    Serial.print(_latest_offset, DEC);
    Serial.print(" latest:");
    Serial.println(_latest_val, DEC);
}

// write a new mileage value, updates the multiplier if need be
void EEPROMStore::writeLatestEEPROM(long val) {
    _latest_val = val;
    // check for case where we have just rolled over, last byte will be marker
    // _latest_offset will be start
    if (_latest_offset == k_start_eeprom_array && EEPROM.read(k_end_of_eeprom-2) == 0x80) {
	EEPROM.write(k_end_of_eeprom - 2, 0);
	Serial.println("blank end of eeprom array");
    }
    EEPROM.update(_latest_offset++, ((val & 0xff00) >> 8));
    EEPROM.update(_latest_offset++, val & 0x00ff);
    EEPROM.write(_latest_offset, 0x80);
    // write the end marker
    // if we've reached the end of eeprom, reset the latest offset
    if (_latest_offset >= k_end_of_eeprom - 2) {
	_latest_offset = k_start_eeprom_array;
    }
    Serial.print("write offset:");
    Serial.print(_latest_offset, DEC);
    Serial.print(" latest:");
    Serial.println(_latest_val, DEC);
}

int EEPROMStore::multiplyMileage(byte multiplier, long val) {
    int result = 0;
    for (int i=0; i<multiplier; ++i)
	result += 32768;
    result += val;
    return result;
}

// find the current mileage stored in EEPROM
void EEPROMStore::readMileage() {
    scanEEPROMForLatest();
    _mileage = multiplyMileage(_header.multiplier, _latest_val);
    _written_mileage = _mileage;
    Serial.print("readMileage:");
    Serial.println(_mileage, DEC);
}

// take total mileage and current multiplier, get an updated multiplier and
// remainder value. Returns true if multiplier was updated
bool EEPROMStore::collapseMileage(int mileage, byte& multiplier, long& val)
{
    bool multiplier_changed = false;
    int cval = mileage;
    for (int i=0; i<multiplier; ++i)
	cval -= 32768;
    while (cval >= 32768) {
	multiplier++;
	multiplier_changed = true;
	cval -= 32768;
    }
    // now that we are under 32768, we can use a long
    val = cval;
    return multiplier_changed;
}

// write the current mileage in the EEPROM, no effect
// if the value is the same as already stored
void EEPROMStore::writeMileage() {
    Serial.print("writeMileage");
    if (_mileage == _written_mileage) {
	Serial.println(" - skip");
	return;
    }
    long newval;
    if (collapseMileage(_mileage, _header.multiplier, newval))
	updateHeader();
    Serial.print(" mult:");
    Serial.print(_header.multiplier, DEC);
    Serial.print(" val:");
    Serial.println(newval, DEC);
    writeLatestEEPROM(newval);
    _written_mileage = _mileage;
}

// return the current mileage
int EEPROMStore::mileage()
{
    return _mileage;
}

// set the current mileage
void EEPROMStore::setMileage(int val)
{
    Serial.print("Set mileage to:");
    Serial.println(val, DEC);
    _written_mileage = _mileage = val;
    long newval;
    byte mult = 0;
    collapseMileage(_mileage, mult, newval);
    writeLatestEEPROM(newval);
    _header.multiplier = mult;
    _header.trip1.multiplier = _header.trip2.multiplier = mult;
    _header.trip1.marker = _header.trip2.marker = newval;
    updateHeader();
}
    
// add value to the mileage
void EEPROMStore::addMileage(int val)
{
    _mileage += val;
}

// get the rpm range
long EEPROMStore::rpmRange()
{
    return _header.rpm_range;
}

// set the rpm range
void EEPROMStore::setRPMRange(long range)
{
    _header.rpm_range = range;
    updateHeader();
}

// get the contrast
uint8_t EEPROMStore::contrast()
{
    return _header.contrast;
}

// set the contrast
void EEPROMStore::setContrast(uint8_t newval)
{
    _header.contrast = newval;
    updateHeader();
}

// get the backlight pwm value
int EEPROMStore::backlight()
{
    return (_header.backlight_hi << 8) + _header.backlight_lo;
}

// set the backlight
void EEPROMStore::setBacklight(int newval)
{
    _header.backlight_hi = (uint8_t)((newval & 0xff00) >> 8);
    _header.backlight_lo = (uint8_t)(newval & 0xff);
    updateHeader();
}

// get the voltage correction value
float EEPROMStore::voltageCorrection()
{
    return _header.voltage_correction;
}

// set the voltage correction
void EEPROMStore::setVoltageCorrection(float newval)
{
    _header.voltage_correction = newval;
    updateHeader();
}

bool EEPROMStore::isMetric()
{
    return (_header.flags & METRIC_FLAG) == METRIC_FLAG;
}

bool EEPROMStore::isImperial()
{
    return !isMetric();
}

void EEPROMStore::setMetric()
{
    _header.flags |= METRIC_FLAG;
    updateHeader();
}

void EEPROMStore::setImperial()
{
    _header.flags &= ~(METRIC_FLAG);
    updateHeader();
}

void EEPROMStore::resetTrip1()
{
    long val;
    byte mult = 0;
    collapseMileage(_mileage, mult, val);
    _header.trip1.multiplier = mult;
    _header.trip1.marker = val;
    updateHeader();
}

void EEPROMStore::resetTrip2()
{
    long val;
    byte mult = 0;
    collapseMileage(_mileage, mult, val);
    _header.trip2.multiplier = mult;
    _header.trip2.marker = val;
    updateHeader();
}

int EEPROMStore::trip1()
{
    int marker_mileage = multiplyMileage(_header.trip1.multiplier,
					 _header.trip1.marker);
    return _mileage - marker_mileage;
}

int EEPROMStore::trip2()
{
    int marker_mileage = multiplyMileage(_header.trip2.multiplier,
					 _header.trip2.marker);
    return _mileage - marker_mileage;
}
