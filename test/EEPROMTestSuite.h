#include <cxxtest/TestSuite.h>
#include <cxxtest/GlobalFixture.h>

#include "EEPROMStore.h"

class Fixture : public CxxTest::GlobalFixture
{
    EEPROMStore* _store;
    
public:
    bool setUpWorld() 
        {
            Serial.begin();
            return true;
        }

    bool tearDownWorld()
        {
            Serial.end();
            return true;
        }
    
    bool setUp() 
        {
            EEPROM.reset();
            _store = new EEPROMStore();
            _store->begin();
            _store->initializeEEPROM();
            return true;
        }

    bool tearDown()
        {
            delete _store;
            return true;
        }
    
    EEPROMStore* store() 
        {
            return _store;
        }
    
};
static Fixture fixture;

class EEPROMTestSuite : public CxxTest::TestSuite
{
public:
    void test_default( void )
        {
            TS_ASSERT_EQUALS( fixture.store()->mileage(), 0 );
            TS_ASSERT_EQUALS( fixture.store()->rpmRange(), 12000 );
            TS_ASSERT_EQUALS( fixture.store()->contrast(), 50 );
            TS_ASSERT_EQUALS( fixture.store()->backlight(), 128 );
            TS_ASSERT_EQUALS( fixture.store()->voltageOffset(), 0.0 );
            TS_ASSERT_EQUALS( fixture.store()->voltageCorrection(), 1.0 );
            TS_ASSERT_EQUALS( fixture.store()->speedoCorrection(), 1.0 );
            TS_ASSERT_EQUALS( fixture.store()->isMetric(), false );
            TS_ASSERT_EQUALS( fixture.store()->isImperial(), true );
            TS_ASSERT_EQUALS( fixture.store()->trip1(), 0 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), 0 );
        }
    
    void test_mileage_set( void )
        {
            unsigned long newm = 4;
            fixture.store()->setMileage(newm);
            TS_ASSERT_EQUALS( fixture.store()->mileage(), newm );
            TS_ASSERT_EQUALS( fixture.store()->trip1(), 0 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), 0 );
            newm = 65536;
            fixture.store()->setMileage(newm);
            TS_ASSERT_EQUALS( fixture.store()->mileage(), newm );
            TS_ASSERT_EQUALS( fixture.store()->trip1(), 0 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), 0 );
        }

    void test_mileage_add( void )
        {
            unsigned long newm = 4;
            fixture.store()->addMileage(newm);
            TS_ASSERT_EQUALS( fixture.store()->mileage(), newm );
            TS_ASSERT_EQUALS( fixture.store()->trip1(), newm );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), newm );
            fixture.store()->addMileage(newm);
            TS_ASSERT_EQUALS( fixture.store()->mileage(), newm*2 );
            TS_ASSERT_EQUALS( fixture.store()->trip1(), newm*2 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), newm*2 );
            newm = 0;
            fixture.store()->setMileage(newm);
            for (int i=0; i<EEPROM.length()*3; ++i) {
                fixture.store()->addMileage(1);
                fixture.store()->writeMileage();
                TS_ASSERT_EQUALS( fixture.store()->mileage(), i + 1 );
                TS_ASSERT_EQUALS( fixture.store()->trip1(), i + 1 );
                TS_ASSERT_EQUALS( fixture.store()->trip2(), i + 1 );
            }
        }

    void test_mileage_set_add( void )
        {
            unsigned long m = 65000;
            fixture.store()->setMileage(m);
            for (int i=0; i<5; ++i)
            {
                fixture.store()->addMileage(1);
                fixture.store()->writeMileage();
            }

            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->mileage(), m + 5 );
            TS_ASSERT_EQUALS( store1->trip1(), 5 );
            TS_ASSERT_EQUALS( store1->trip2(), 5 );
        }

    void test_mileage_rollover( void )
        {
            unsigned long m = 255 * 0x8fff + 0x8ffe;
            fixture.store()->setMileage(m);
            for (int i=0; i<3; ++i)
            {
                fixture.store()->addMileage(1);
                fixture.store()->writeMileage();
            }

            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->mileage(), 1 );
            TS_ASSERT_EQUALS( store1->trip1(), 3 );
            TS_ASSERT_EQUALS( store1->trip2(), 3 );
        }

    void test_mileage_write( void )
        {
            fixture.store()->addMileage(4);
            fixture.store()->writeMileage();
            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->mileage(), 4 );
        }

    void test_rpmrange( void )
        {
            word rng = 8000;
            fixture.store()->setRPMRange(rng);
            TS_ASSERT_EQUALS( fixture.store()->rpmRange(), rng );
            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->rpmRange(), rng );
        }

    void test_contrast( void )
        {
            uint8_t con = 25;
            fixture.store()->setContrast(con);
            TS_ASSERT_EQUALS( fixture.store()->contrast(), con );
            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->contrast(), con );
        }

    void test_backlight( void )
        {
            int bl = 25;
            fixture.store()->setBacklight(bl);
            TS_ASSERT_EQUALS( fixture.store()->backlight(), bl );
            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->backlight(), bl );
        }

    void test_voltageoffset( void )
        {
            float off = 4.5;
            fixture.store()->setVoltageOffset(off);
            TS_ASSERT_EQUALS( fixture.store()->voltageOffset(), off );
            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->voltageOffset(), off );
        }

    void test_voltagecorrection( void )
        {
            float corr = 1.5;
            fixture.store()->setVoltageCorrection(corr);
            TS_ASSERT_EQUALS( fixture.store()->voltageCorrection(), corr );
            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->voltageCorrection(), corr );
        }

    void test_speedocorrection( void )
        {
            float corr = 1.5;
            fixture.store()->setSpeedoCorrection(corr);
            TS_ASSERT_EQUALS( fixture.store()->speedoCorrection(), corr );
            EEPROMStore* store1 = new EEPROMStore();
            store1->begin();
            TS_ASSERT_EQUALS( store1->speedoCorrection(), corr );
        }

    void test_units( void )
        {
            TS_ASSERT( !fixture.store()->isMetric() );
            TS_ASSERT( fixture.store()->isImperial() );
        }

    void test_set_units( void )
        {
            fixture.store()->setMetric();
            
            TS_ASSERT( fixture.store()->isMetric() );
            TS_ASSERT( !fixture.store()->isImperial() );

            fixture.store()->setImperial();
            
            TS_ASSERT( !fixture.store()->isMetric() );
            TS_ASSERT( fixture.store()->isImperial() );
        }

    void test_reset_trip( void )
        {
            unsigned long m = 1000;
            fixture.store()->setMileage(m);
            fixture.store()->writeMileage();

            TS_ASSERT_EQUALS( fixture.store()->trip1(), 0 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), 0 );

            for (int i=0; i<5; ++i)
                fixture.store()->addMileage(5);

            TS_ASSERT_EQUALS( fixture.store()->trip1(), 5*5 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), 5*5 );

            fixture.store()->resetTrip1();
            
            for (int i=0; i<5; ++i)
                fixture.store()->addMileage(5);

            TS_ASSERT_EQUALS( fixture.store()->trip1(), 5*5 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), 5*5 + 5*5 );

            fixture.store()->resetTrip2();
            
            for (int i=0; i<5; ++i)
                fixture.store()->addMileage(5);

            TS_ASSERT_EQUALS( fixture.store()->trip1(), 5*5 + 5*5 );
            TS_ASSERT_EQUALS( fixture.store()->trip2(), 5*5 );
        }
    
};
