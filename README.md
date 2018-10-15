# EEPROMStore Library

Library provides interface to store and retrieve Speedometer project data from the Arduino EEPROM.

Written by Greg Green
GPL v3.0 license, see [LICENSE](https://github.com/gpgreen/EEPROMStore/blob/master/LICENSE)

The speedometer project is an Arduino [sketch](https://github.com/gpgreen/motorcycle_instruments)

## Testing

The directory 'test' contains code to test the library. It uses the CxxTest framework to build and run the tests. It also uses gcov and lcov to instrument code coverage.

To install these libraries on ubuntu:

`sudo apt install cxxtest gcov lcov`

There is a Makefile in the directory

`make gcov`