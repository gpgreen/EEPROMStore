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

#if defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
  int k_end_of_eeprom = 2048;
#elif defined(__arm__) && defined(TEENSYDUINO)
  // eeprom library not working after length of 1024 bytes, even though the teensy 3.0 has 2048
  int k_end_of_eeprom = 1024;
#else
  #error "Unknown chip, need to specify with eeprom size"
#endif

// The constructor
EEPROMStore::EEPROMStore()
    : _latest_offset(0), _latest_val(0), _mileage(0L), _written_mileage(0L)
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
    _header.rpm_range = 12000;
    _header.contrast = 50;
    _header.multiplier = 0;
    _header.backlight_hi = 0;
    _header.backlight_lo = 128;
    _header.voltage_offset = 0.0;
    _header.voltage_correction = 1.0;
    _header.speedo_correction = 1.0;
    _header.trip1.multiplier = 0;
    _header.trip1.marker = 0;
    _header.trip2.multiplier = 0;
    _header.trip2.marker = 0;
}

// read the header field from the EEPROM
// this contains rarely written values
void EEPROMStore::readEEPROMHeader()
{
    Serial.print("EEPROM size:");
    Serial.println(EEPROM.length(), DEC);

    EEPROM.get(0, _header);

    Serial.print("EEPROM Header version:");
    Serial.println(_header.version, DEC);
    if (_header.version != 0)
    {
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
    Serial.println("]");

    Serial.print("[volt off:");
    Serial.print(_header.voltage_offset, 6);

    Serial.print(" volt corr:");
    Serial.print(_header.voltage_correction, 6);

    Serial.print(" speed corr:");
    Serial.print(_header.speedo_correction, 6);
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
void EEPROMStore::updateHeader()
{
    Serial.println("updateHeader");
    EEPROM.put(0, _header);
}

// initialize the eeprom to it's starting state with zero mileage
void EEPROMStore::initializeEEPROM()
{
    Serial.println("initializeEEPROM");
    resetHeader();
    updateHeader();
    for (int i=k_start_eeprom_array; i<k_end_of_eeprom; ++i)
        EEPROM.write(i, 0);
    _mileage = _written_mileage = 0L;
}

// read the EEPROM value array to get the latest mileage value
void EEPROMStore::scanEEPROMForLatest()
{
    byte b;
    _latest_offset = k_start_eeprom_array;
    Serial.print("Scan eeprom for end marker");
    Serial.print(" at offset:");
    Serial.println(_latest_offset, DEC);
    for (int i=_latest_offset; i<k_end_of_eeprom; ++i)
    {
        b = EEPROM.read(i);
#if defined(SERIAL_DEBUG_MSG)
        Serial.print("b:");
        Serial.print(b, HEX);
        Serial.print(" offset:");
        Serial.print(i, DEC);
#endif
        if ((b & 0x80) == 0)
        {
            _latest_val = (b << 8) + EEPROM.read(++i);
#if defined(SERIAL_DEBUG_MSG)
            Serial.print(" val:");
            Serial.println(_latest_val, DEC);
#endif
        }
        else
        {
            _latest_val = ((b & 0x7f) << 8) + EEPROM.read(++i);
            break;
        }
        _latest_offset += 2;
    }
    // special case is EEPROM all 0, no values written yet
    if (_latest_offset >= k_end_of_eeprom)
    {
        _latest_offset = k_start_eeprom_array;
        _latest_val = 0;
#if defined(SERIAL_DEBUG_MSG)
        Serial.println("\nblank mileage");
#endif
    }
    Serial.print("\nwrite offset:");
    Serial.print(_latest_offset, DEC);
    Serial.print(" latest:");
    Serial.println(_latest_val, DEC);
}

// write a new mileage value, updates the multiplier if need be
void EEPROMStore::writeLatestEEPROM(word val)
{
#if defined(SERIAL_DEBUG_MSG)
    Serial.println("writeLatestEEPROM");
#endif
    _latest_val = val;
    // check for case where we have are at beginning of eeprom array, don't
    // need to rewrite the marker byte
    bool rewrite = true;
    if (_latest_offset == k_start_eeprom_array)
    {
        rewrite = false;
    }
    byte old_marker = rewrite ? EEPROM.read(_latest_offset - 2) : 0;
    // if the mcu is turned off here before it is able to finish writing we could
    // get a corrupted flash, so write the new word first, and don't rewrite
    // the existing marker byte till last. The additional marker byte won't get found
    // before the existing one if it hasn't yet been overwritten
    EEPROM.update(_latest_offset++, ((val & 0xff00) >> 8) | 0x80);
    EEPROM.update(_latest_offset++, val & 0x00ff);
    // write over the old marker, unless it was at end of eeprom
    if (rewrite)
    {
        EEPROM.write(_latest_offset - 4, old_marker & 0x7f);
    }
    // if we've reached the end of eeprom, reset offset
    if (_latest_offset >= k_end_of_eeprom - 2)
    {
        _latest_offset = k_start_eeprom_array;
    }
#if defined(SERIAL_DEBUG_MSG)
    Serial.print("write offset:");
    Serial.print(_latest_offset, DEC);
    Serial.print(" latest:");
    Serial.println(_latest_val, DEC);
#endif
}

unsigned long EEPROMStore::multiplyMileage(byte multiplier, word val)
{
    unsigned long result = multiplier * 0x8fff;
    result += val;
    return result;
}

// find the current mileage stored in EEPROM
void EEPROMStore::readMileage()
{
    scanEEPROMForLatest();
    _mileage = multiplyMileage(_header.multiplier, _latest_val);
    _written_mileage = _mileage;
    Serial.print("readMileage:");
    Serial.println(_mileage, DEC);
}

// take total mileage and current multiplier, get an updated multiplier and
// remainder value. Returns true if multiplier was updated
bool EEPROMStore::collapseMileage(unsigned long mileage, byte& multiplier, word& val)
{
    bool multiplier_changed = false;
    unsigned long cval = mileage - (multiplier * 0x8fff);
    while (cval > 0x8fff)
    {
        ++multiplier;
        multiplier_changed = true;
        cval -= 0x8fff;
    }
    // now that we are under 0x8fff, we can use a word
    val = cval;
    return multiplier_changed;
}

// write the current mileage in the EEPROM, no effect
// if the value is the same as already stored
void EEPROMStore::writeMileage()
{
#if defined(SERIAL_DEBUG_MSG)
    Serial.println("writeMileage");
#endif
    if (_mileage == _written_mileage)
    {
#if defined(SERIAL_DEBUG_MSG)
        Serial.println(" - skip");
#endif
        return;
    }
    word newval;
    byte old_mult = _header.multiplier;
    if (collapseMileage(_mileage, _header.multiplier, newval))
    {
        // rollover case
        if (old_mult > _header.multiplier)
        {
            _mileage = 0;
            newval = 0;
#if defined(SERIAL_DEBUG_MSG)
            Serial.println("### rollover ###");
#endif
        }
        updateHeader();
    }
#if defined(SERIAL_DEBUG_MSG)
    Serial.print("mileage:");
    Serial.print(_mileage, DEC);
    Serial.print(" mult:");
    Serial.print(_header.multiplier, DEC);
    Serial.print(" val:");
    Serial.println(newval, DEC);
#endif
    writeLatestEEPROM(newval);
    _written_mileage = _mileage;
}

// return the current mileage
unsigned long EEPROMStore::mileage()
{
    return _mileage;
}

// set the current mileage
void EEPROMStore::setMileage(unsigned long val)
{
    Serial.print("Set mileage to:");
    Serial.println(val, DEC);
    _written_mileage = _mileage = val;
    word newval;
    byte mult = 0;
    collapseMileage(_mileage, mult, newval);
    writeLatestEEPROM(newval);
    _header.multiplier = mult;
    _header.trip1.multiplier = _header.trip2.multiplier = mult;
    _header.trip1.marker = _header.trip2.marker = newval;
    updateHeader();
}
    
// add value to the mileage
void EEPROMStore::addMileage(unsigned long val)
{
    _mileage += val;
}

// get the rpm range
word EEPROMStore::rpmRange()
{
    return _header.rpm_range;
}

// set the rpm range
void EEPROMStore::setRPMRange(word range)
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
    _header.backlight_hi = static_cast<uint8_t>((newval & 0xff00) >> 8);
    _header.backlight_lo = static_cast<uint8_t>(newval & 0xff);
    updateHeader();
}

// get the voltage offset value
float EEPROMStore::voltageOffset()
{
    return _header.voltage_offset;
}

// set the voltage offset
void EEPROMStore::setVoltageOffset(float newval)
{
    _header.voltage_offset = newval;
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

// get the speedo correction value
float EEPROMStore::speedoCorrection()
{
    return _header.speedo_correction;
}

// set the speedo correction
void EEPROMStore::setSpeedoCorrection(float newval)
{
    _header.speedo_correction = newval;
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
    word val;
    byte mult = 0;
    collapseMileage(_mileage, mult, val);
    _header.trip1.multiplier = mult;
    _header.trip1.marker = val;
    updateHeader();
}

void EEPROMStore::resetTrip2()
{
    word val;
    byte mult = 0;
    collapseMileage(_mileage, mult, val);
    _header.trip2.multiplier = mult;
    _header.trip2.marker = val;
    updateHeader();
}

unsigned long EEPROMStore::trip1()
{
    unsigned long marker_mileage = multiplyMileage(_header.trip1.multiplier,
                                                   _header.trip1.marker);
    if (_mileage < marker_mileage)
        // handle rollover
        return _mileage + 9436929 - marker_mileage;
    return _mileage - marker_mileage;
}

unsigned long EEPROMStore::trip2()
{
    unsigned long marker_mileage = multiplyMileage(_header.trip2.multiplier,
                                                   _header.trip2.marker);
    if (_mileage < marker_mileage)
        // handle rollover
        return _mileage + 9436929 - marker_mileage;
    else
        return _mileage - marker_mileage;
}
