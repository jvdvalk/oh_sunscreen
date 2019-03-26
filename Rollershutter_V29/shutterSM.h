 /* *****************************************************************************************************************************
 * Rollershutter SM
 * 
 * Created by (2016) Pascal Moreau / Scalz
 *
 * ***************************************************************************************************************************** */
#ifndef shutterSM_h
#define shutterSM_h

// ********************* PIN/RELAYS DEFINES *******************************************
#define SHUTTER_UPDOWN_PIN        19                    // Default pin for relay SPDT, you can change it with : initShutter(SHUTTER_POWER_PIN, SHUTTER_UPDOWN_PIN)
#define SHUTTER_POWER_PIN         18                    // Default pin for relay SPST : Normally open, power off rollershutter motor

#define RELAY_ON                  0                     // ON state for power relay
#define RELAY_OFF                 1
#define RELAY_UP                  0                     // UP state for UPDOWN relay
#define RELAY_DOWN                1                     // UP state for UPDOWN relay

// ********************* EEPROM DEFINES **********************************************
#define ADDR_UP                   100
#define ADDR_DWN                  101

// ********************* CALIBRATION DEFINES *****************************************
#define START_POSITION            100                     // Shutter start position in % if no parameter in memory

#define TRAVELTIME_UP             28               // in sec, time measured between 0-100% for Up. it's initialization value. if autocalibration used, these will be overriden
#define TRAVELTIME_DOWN           23               // in sec, time measured between 0-100% for Down

#endif
