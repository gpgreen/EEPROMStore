#ifndef EEPROM_H_
#define EEPROM_H_

#include <Arduino.h>
#include <vector>
#include <stdexcept>

class MockEEPROM
{
public:
    MockEEPROM(size_t sz)
        : len(sz)
        {
            mem.assign(len, 0);
        }
    
    int length()
        {
            return len;
        }

    /*
     * reset the eeprom memory to all zero's
     */
    void reset()
        {
            mem.assign(len, 0);
        }
    
    template< typename T > T& get(int idx, T& val)
        {
            int l = static_cast<int>(len - sizeof(T));
            if (idx >= 0 && idx <= l)
            {
                byte* p = reinterpret_cast<byte*>(&val);
                for (size_t i=0; i<sizeof(val); ++i)
                    *(p + i) = mem[idx+i];
                return val;
            }
            else
                throw std::runtime_error("eeprom overflow");
        }

    template< typename T > void put(int idx, T& val)
        {
            int l = static_cast<int>(len - sizeof(T));
            if (idx >= 0 && idx <= l)
            {
                byte* p = reinterpret_cast<byte*>(&val);
                for (size_t i=0; i<sizeof(val); ++i)
                    mem[idx+i] = *(p + i);
            }
            else
                throw std::runtime_error("eeprom overflow");
        }

    byte read(int idx)
        {
            byte b;
            return get(idx, b);
        }

    void write(int idx, byte b)
        {
            put(idx, b);
        }

    void update(int idx, byte b)
        {
            put(idx, b);
        }

    bool compare(std::vector<byte>& shouldbe)
        {
            for (size_t i=0; i<len; ++i)
                if (shouldbe[i] != mem[i])
                    return false;
            return true;
        }
    
    size_t len;
    std::vector<byte> mem;
};

extern MockEEPROM EEPROM;

#endif
