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

// eeprom library not working after length of 1024 bytes
int k_end_of_eeprom = 1024;

// The constructor
EEPROMStore::EEPROMStore()
    : _latest_offset(0), _latest_val(0), _hi_bit(0), _mileage(0), _written_mileage(0)
{
    resetHeader();
}

void EEPROMStore::begin()
{
    readEEPROMHeader();
    readMileage();
}

void EEPROMStore::resetHeader()
{
    _header.version = 0;
    _header.hi_byte_rpm_range = 0x2e;
    _header.lo_byte_rpm_range = 0xe0;
    _header.contrast = 50;
    _header.multiplier = 0;
}

// read the header field from the EEPROM
// this contains rarely written values
void EEPROMStore::readEEPROMHeader() {
    Serial.print("EEPROM size:");
    Serial.println(EEPROM.length(), DEC);

    _header.version = EEPROM.read(0);
    Serial.print("EEPROM Header version:");
    Serial.println(_header.version, DEC);
    if (_header.version != 0) {
        initializeEEPROM();
        Serial.println("Reinitialized EEPROM as it was formatted incorrectly");
    }

    _header.hi_byte_rpm_range = EEPROM.read(1);
    Serial.print("EEPROM Header hi byte rpm range:");
    Serial.print(_header.hi_byte_rpm_range, HEX);
    _header.lo_byte_rpm_range = EEPROM.read(2);
    Serial.print(" lo byte rpm range:");
    Serial.print(_header.lo_byte_rpm_range, HEX);

    _header.contrast = EEPROM.read(3);
    Serial.print(" contrast:");
    Serial.print(_header.contrast, DEC);

    _header.multiplier = EEPROM.read(4);
    Serial.print(" multiplier:");
    Serial.println(_header.multiplier, HEX);
}

// initialize the eeprom to it's starting state with zero mileage
void EEPROMStore::initializeEEPROM() {
    Serial.println("initializeEEPROM");
    resetHeader();
    EEPROM.write(0, _header.version);
    EEPROM.write(1, _header.hi_byte_rpm_range);
    EEPROM.write(2, _header.lo_byte_rpm_range);
    EEPROM.write(3, _header.contrast);
    // set the multiplier
    byte* offset = &_header.multiplier;
    EEPROM.write(reinterpret_cast<int>(offset), _header.multiplier);
    for (int i=k_start_eeprom_array; i<k_end_of_eeprom; ++i)
        EEPROM.write(i, 0);
    _mileage = _written_mileage = 0;
}

// read the EEPROM value array to get the latest mileage value
void EEPROMStore::scanEEPROMForLatest() {
    byte b;
    _latest_offset = k_start_eeprom_array;
    _hi_bit = EEPROM.read(_latest_offset) & 0x80;
    Serial.print("Scan eeprom with flag:");
    Serial.println(_hi_bit, HEX);
    for (int i=_latest_offset; i<k_end_of_eeprom - 1; ++i) {
        _latest_offset += 2;
        b = EEPROM.read(i);
        if ((b & 0x80) == _hi_bit) {
            _latest_val = ((b & 0x7) << 8);
            _latest_val += EEPROM.read(++i);
            Serial.print(i, DEC);
            Serial.print(":");
            Serial.println(_latest_val, DEC);
        } else {
            break;
        }
        Serial.print("offset:");
        Serial.println(_latest_offset, DEC);
    }
    // special case is all values are set with latest flag, so handle
    if (_latest_offset == k_end_of_eeprom) {
        _hi_bit ^= 0x80;
        _latest_offset = k_start_eeprom_array;
        Serial.print("flip flag:");
        Serial.println(_hi_bit, HEX);
    }
    Serial.print("write offset:");
    Serial.print(_latest_offset, DEC);
    Serial.print(" latest:");
    Serial.println(_latest_val, DEC);
}

// write a new mileage value, updates the multiplier if need be
void EEPROMStore::writeLatestEEPROM(long val) {
    Serial.print(" hi(");
    Serial.print(_hi_bit, HEX);
    Serial.print(":");
    Serial.print(_latest_offset, DEC);
    Serial.print("):");
    Serial.println(val, DEC);
    _latest_val = val;
    EEPROM.update(_latest_offset++, ((val >> 8) & 0x7) | _hi_bit);
    EEPROM.update(_latest_offset++, val & 0xf);
    // if we've reached the end of eeprom, flip the hi bit flag
    if (_latest_offset == k_end_of_eeprom) {
        _hi_bit ^= 0x80;
        _latest_offset = k_start_eeprom_array;
    }
}

// find the current mileage stored in EEPROM
void EEPROMStore::readMileage() {
    scanEEPROMForLatest();
    for (int i=0; i<_header.multiplier; ++i)
        _mileage += 32768;
    _mileage += _latest_val;
    _written_mileage = _mileage;
    Serial.print("readMileage:");
    Serial.println(_mileage, DEC);
}

// write the current mileage in the EEPROM, no effect
// if the value is the same as already stored
void EEPROMStore::writeMileage() {
    Serial.print("writeMileage");
    if (_mileage == _written_mileage) {
        Serial.println(" - skip");
        return;
    }
    long newval = _mileage;
    for (int i=0; i<_header.multiplier; ++i)
        newval -= 32768;
    if (newval >= 32768) {
        _header.multiplier++;
        EEPROM.update(3, _header.multiplier);
        newval -= 32768;
    }
    writeLatestEEPROM(newval);
    _written_mileage = _mileage;
}

// return the current mileage
long EEPROMStore::mileage()
{
	return _mileage;
}

// add value to the mileage
void EEPROMStore::addMileage(long val)
{
	_mileage += val;
}

// get the rpm range
long EEPROMStore::rpmRange()
{
    return (_header.hi_byte_rpm_range << 8) + _header.lo_byte_rpm_range;
}

// get the contrast
uint8_t EEPROMStore::contrast()
{
    return _header.contrast;
}
