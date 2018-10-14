###########################################################################
# Makefile for EEPROMStore test
###########################################################################
# all:	 builds and executes test
# gcov:  cleans, builds, executes using gcov and lcov
# clean: removes all non-source files

###########################################################################
# variables
###########################################################################

# compiler and linker flags
CPPFLAGS = -MD -MP -I. -I../src/ -DARDUINO=100 -D__AVR_ATmega644__
CXXFLAGS = -g -W -Wall -Werror -fprofile-arcs -ftest-coverage
LDFLAGS = -g -fprofile-arcs -ftest-coverage

# source files
SOURCES = EEPROMStore.cpp Serial.cpp EEPROM.cpp tests.cpp

# object files
OBJECTS = $(SOURCES:.cpp=.o)

# look in the src directory for files under test
VPATH = ../src

###########################################################################
# targets
###########################################################################

all: run

# the CxxTest generated test source
tests.cpp: EEPROMTestSuite.h
	cxxtestgen --error-printer -o $@ $<

# dependency files
DEPENDS = $(SOURCES:.cpp=.d)
-include $(DEPENDS)
%.d:
	@touch $@

# test executable
main: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

# run the tests
run: main
	./main

gcov: clean main run
	gcov -b $(SOURCES)
	lcov --capture --directory . --output-file main_coverage.info
	genhtml main_coverage.info --output-directory out

# clean
.PHONY : clean
clean:
	-rm -rf tests.cpp $(OBJECTS) main *.d *.log  main_coverage.info *.gcda *.gcno out
