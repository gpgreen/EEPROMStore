#ifndef SERIAL_HPP_
#define SERIAL_HPP_

#include <iostream>
#include <iomanip>

enum INT_FORMAT {DEC, HEX};

class MockSerial
{
    std::ostream& os;
    
public:

    MockSerial(std::ostream& s) : os(s) 
        {
        }

    static void begin();
    static void end();
    
    void print(const char* msg)
        {
            os << msg;
        }

    void print(int num, INT_FORMAT fmt = DEC)
        {
            if (fmt == DEC)
                os << std::dec << num;
            else if (fmt == HEX)
                os << std::hex << num;
        }
    
    void print(float num, int places)
        {
            os << std::fixed << std::setprecision(places) << num;
        }
    
    void println(const char* msg)
        {
            os << msg << std::endl; 
        }

    void println(int num, INT_FORMAT fmt = DEC)
        {
            print(num, fmt);
            os << std::endl; 
        }
    
};

#endif
