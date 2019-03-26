 /* *****************************************************************************************************************************
 * Rollershutter Node Definition
 * 
 * Created by (2016) Pascal Moreau / Scalz
 * 
 * ******************************************************************************************************************************/

#ifndef MyNodeDefinition_h
#define MyNodeDefinition_h

/* *******************************************************************************************************************
 *                                          MYSENSORS 
 * ******************************************************************************************************************/
// Enable and select radio type attached
#define MY_RF24_PA_LEVEL RF24_PA_HIGH   // LOW ; HIGH ; MAX
#define MY_RADIO_RF24

#define MY_DEBUG // Enable Mysensors debug prints to serial monitor, comment it to save memory

/* *******************************************************************************************************************
 *                                          PIN MAPPING 
 * ******************************************************************************************************************/

#define WINDOW_PIN        2    // Digital In  pin number for window open contact
#define RELAY_UPDOWN      3    // Digital In  pin number for relay SPDT
#define RELAY_POWER       4    // Digital In  pin number for relay SPST : Normally open, power off rollershutter motor, if A2==1 and A1==0 moveup, if A2==1 and A1==1 movedown, if A2==0 power off
#define DEBUG_LED        13    // Digital In  pin number for onboard debug led
#define BUTTON_STOP       5    // Digital In  pin number for button Stop
#define BUTTON_UP         6    // Digital In  pin number for button Up
#define BUTTON_DOWN       7    // Digital In  pin number for button Down
#define LED               8    // Digital Out pin number for front LED
#define CALIBRATION_PIN   A0   // Digital In  pin number for calibration up en down

/* *******************************************************************************************************************
 *                                          NODE 
 * ******************************************************************************************************************/
//#define MY_REPEATER_FEATURE   // Enable repeater functionality for this node

#define RELEASE           "2.9"
#define NODE_NAME         "RollerShutter 25-3-19 "
#define MY_DEBUG_SKETCH                               // Enable specific sketch debug prints to serial monitor, comment it to save memory

#define CHILD_ID_ROLLERSHUTTER    1                   // Rollershutter UP/DOWN/STOP/PERCENT
#define CHILD_ID_WINDOW           2                   // door          TRIPPED/ARMED
#define CHILD_ID_BLOCKDOWN        5                   // kan het, met de hand, naar beneden lopen van het scherm blokkeren via OH
#define CHILD_ID_GETVALUES      250                   // special, alle waarden worden naar OH gestuurd

// ********************* BUTTONS STATES **********************************************
#define BT_PRESS_NONE             0                   // geen toets ingedrukt
#define BT_PRESS_UP               1                   // inlopen / omhoog toets
#define BT_PRESS_DOWN             2                   // uitlopen / omlaag toets
#define BT_PRESS_STOP             3                   // stop toets
#define CAL_ACTIVE                4                   // calibratie aktief
#define WINDOW_OPEN               5                   // raam staat open, blokkeer scherm

#endif
