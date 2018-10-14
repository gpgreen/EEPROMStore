#include "Serial.hpp"
#include <fstream>

static std::ofstream serial_out;

void MockSerial::begin() 
{
    serial_out.open("Serial.log");
}

void MockSerial::end()
{
    serial_out.close();
}

MockSerial Serial(serial_out);

