/* *****************************************************************************************************************************
 * ROLLER SHUTTER NODE
 * Created by (2019) Jos van der Valk
 * 
 * 
 * Version 2.9 JV 25-03-2019  delay tussen up/down keuze en motor start gezet mogelijk probleem met defect up/down relais op te lossen
 * Version 2.8 JV 12-02-2019  Calibratie uitgeschakeld nu via vaste waarden !! TODO 
 * Version 2.7 JV 02-02-2019  Led om aan te geven dat BlockDown of Window blokkade aanwezig is
 * Version 2.6 JV 27-01-2019  BlockDown toegevoegd OH kan nu het uitlopen blokkeren ivm regen, wind
 * Version 2.5 JV 21-01-2019  GetValues toegevoegd OH kan nu, bij opstarten, alle waarden opvragen
 * Version 2.4 JV 20-01-2019  Bugfix: verkeerde waarde naar OH bij indrukken van up / down toets
 * Version 2.3 JV 17-01-2019  raam contact toegevoegd ( shutter beweegt niet als raam open is )
 * Version 2.2 JV 17-01-2019  na calibratie direct nieuwe waarde beschikbaar
 * Version 2.1 JV 16-01-2019  mogelijkheid om looptijden dynamisch in te stellen
 * Version 2.0 JV 15-01-2019  eerste versie werkend
 * 
 * For Mysensors community :)
 * 
 * DESCRIPTION : Roller shutter node for Mysensors lib.
 * - opening 0-100%
 * -  types of calibration : 
 *      1) manual       : measure by yourself your up an down delays, and fill values in corresponding DEFINE. 
 *      2) dynamic      : dmv pin laag te maken en scherm op tijd te laten stoppen
 *      
 * - controlled by : Controller, Buttons
 *
 * - and authentication features
 * 
 * Calibreren:
 *  Verbind ingang CALIBRATION_PIN met gnd
 *  Druk op up of down knop 
 *  Op het moment dat scherm op het einde is op stop knop indrukken
 *  Tijd tussen knopppen indrukken wordt opgeslagen in EEPROM
 * 
 * 
 * 
 * 
 * PIN MAP
 * 
 * 3 : Up or Down Relay,  (SPDT 10A) * 
 * 4 : Motor Relay Power, ON/OFF motor(SPST-NO 10A)
 * 
 * 5:  Stop Button (ACTIVE LOW)
 * 6 : Up Button   (ACTIVE LOW) 
 * 7 : Down Button (ACTIVE LOW)
 * 
 * 8 : led op front
 * 9 : jumper tbv calibratie
 * 
 * Libs used :  - Mysensors 2.0               
 *              - Bounce2
 *              - SimpleTimer
 *              
 *              
 * ***************************************************************************************************************************** */ 

/* 
TODO LIST :
- add a way to open/close all shutters from the buttons
- etc.
*/

#include "MyNodeDefinition.h"
#include "shutterSM.h"

#include <SimpleTimer.h>
#include <MySensors.h>
#include <Bounce2.h>
#include <EEPROM.h>

/* *****************************************************************************************************************************
 *  Globals
 * ***************************************************************************************************************************** */

// Buttons
Bounce debounceUp    = Bounce(); 
Bounce debounceDown  = Bounce();
Bounce debounceStop  = Bounce();
Bounce debounceWindow = Bounce();

// MYSENSORS
MyMessage msgUp(CHILD_ID_ROLLERSHUTTER,V_UP);                         // Message for Up 
MyMessage msgDown(CHILD_ID_ROLLERSHUTTER,V_DOWN);                     // Message for Down 
MyMessage msgStop(CHILD_ID_ROLLERSHUTTER,V_STOP);                     // Message for Stop 
MyMessage msgShutterPosition(CHILD_ID_ROLLERSHUTTER,V_PERCENTAGE);    // Message for % shutter 
MyMessage msgWindowOpen(CHILD_ID_WINDOW, V_TRIPPED);                  // Message for window contact
MyMessage msgBlockDown(CHILD_ID_BLOCKDOWN, V_STATUS);                 // Message for blocking because of rain and wind
MyMessage msgGetValues(CHILD_ID_GETVALUES, V_STATUS);                 // Message for retrieving values during startup OH
uint8_t   errorCode               = 0;                                // Node Error code, check table

SimpleTimer timer ; // 4 aparte timers voor omhoog, omlaag, led en calibratie

int timupId  ;
int timdownId;
int timcalId ;
int timLEDId ;

int timCal ;        // waarde welke de looptijd van scherm aangeeft
int sendPerc ;      // onthou welke waarde verstuurd is, zodat het maar 1x verstuurd wordt
static int perc_cur;
static int perc_new;

enum State { noinit, cal_up, cal_dwn, cal_stop, start_up, run_up, start_dwn,
 run_dwn, start_stop, run_stop, powerup, start_windowopen, run_windowopen } ;
State Action = noinit;

enum LedState { off, slow, fast, on };
LedState LedAction = off;
int ledtel;

bool BlockDown = false; // evt blokkeren van het neerlopen van het scherm ivm met regen en wind

/* ======================================================================
Function: before()
Purpose : before Mysensors init
Comments: 
====================================================================== */
void before() {
  analogReference(EXTERNAL);

  digitalWrite(DEBUG_LED, HIGH);
  pinMode(DEBUG_LED, OUTPUT);
  
  // Setup the button
  pinMode(BUTTON_UP   ,INPUT_PULLUP);
  pinMode(BUTTON_DOWN ,INPUT_PULLUP);
  pinMode(BUTTON_STOP ,INPUT_PULLUP);

  // Setup window contact
  pinMode(WINDOW_PIN,  INPUT_PULLUP);
  
  // After setting up the button, setup debouncer
  debounceUp.attach(BUTTON_UP);
  debounceUp.interval(5);
  debounceDown.attach(BUTTON_DOWN);
  debounceDown.interval(5);
  debounceStop.attach(BUTTON_STOP);
  debounceStop.interval(5);
  debounceWindow.attach(WINDOW_PIN);
  debounceWindow.interval(5);
 
  // Set relay pins in output mode
  // Make sure relays are off when starting up
  digitalWrite(RELAY_POWER, RELAY_OFF);
  pinMode(RELAY_POWER, OUTPUT);
  wait(250);                              // V2.29 25-03-2019
  digitalWrite(RELAY_UPDOWN, RELAY_OFF);
  pinMode(RELAY_UPDOWN, OUTPUT);


  // set calibration in input mode
  pinMode(CALIBRATION_PIN, INPUT_PULLUP);
  
  // Setup LED output
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LED_OFF);
}


/* ======================================================================
Function: presentation
Purpose : Send sketch info and present child ids to the GW&Controller 
Comments: 
====================================================================== */
void presentation()  {
  
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(NODE_NAME, RELEASE);
  
  // Register all id (they will be created as child devices)
  present(CHILD_ID_ROLLERSHUTTER, S_COVER);     // V_UP, V_DOWN, V_STOP, V_PERCENTAGE  
  present(CHILD_ID_WINDOW, S_BINARY);
  present(CHILD_ID_GETVALUES, S_BINARY); 
}

/* ======================================================================
Function: setup
Purpose : Arduino Setup 
Comments: 
====================================================================== */
void setup() { 
  #ifdef MY_DEBUG_SKETCH
    Serial.begin(115200);
    Serial.println(" Rollershutter opgestart");
  #endif

//  EEPROM.write(ADDR_DWN, 100);
//  EEPROM.write(ADDR_DWN, 120);
  
  // bepaal de snelheid van tellen
  settimers();
        
  if (raamdicht()){   // raam dicht
    Action = powerup; 
  }
  else{
    Action = start_windowopen;
    timer.disable(timdownId);               // stop scherm down timer
    timer.disable(timupId);                 // stop scherm up timer 
  }   
}



/* ======================================================================
Function: settimer
Purpose : bepalen van delay voor up en down 
Comments: 
====================================================================== */
void settimers() {
   // timed actions setup
   int tempDown = EEPROM.read(ADDR_DWN);
   int tempUp  = EEPROM.read(ADDR_UP);
   
  if ((tempDown > 250)||(tempDown < 20)){                           // waarde in eeprom is onbetrouwbaar, > 250 sec. of < 20 sec.
    timdownId = timer.setInterval(10 * TRAVELTIME_DOWN, TimDown); 
  }
  else { 
    timdownId = timer.setInterval(10*(EEPROM.read(ADDR_DWN)), TimDown);}

    if ((tempUp > 250)||(tempUp < 20)){ // waarde in eeprom is onbetrouwbaar, > 250 sec. of < 20 sec
      timupId   = timer.setInterval(10*TRAVELTIME_UP, TimUp); }
    else {
      timupId   = timer.setInterval(10*(EEPROM.read(ADDR_UP)) , TimUp); }
      
  timLEDId  = timer.setInterval(300, TimLed);      // zet led timer op ,3 seconde

  timcalId = timer.setInterval(1000, TimCal);       // zet Caltimer op 1 seconde
  timer.disable(timcalId);
  
  #ifdef MY_DEBUG_SKETCH
     Serial.println("!!!!!!!!! SETTIMERS");
     Serial.print("up waarde ");
     Serial.println(timupId);
     Serial.print("down waarde ");
     Serial.println(timdownId);
  #endif

}
 
/* ======================================================================
Function: loop
Purpose : Arduino main loop 
Comments: 
====================================================================== */
void loop() {

  // this is where the "polling" occurs
  timer.run();
  
  // Read buttons, interface
  uint8_t buttonPressed = 0;
  buttonPressed = processButtons(); // kijk of en welke toets is ingedrukt

  if (calibratie() == false){
    timer.disable(timcalId);                          // zet calibratie timer stil
    Button_gedrukt(buttonPressed);
  }  
  else { // calibration aktief
    Button_Calibratie(buttonPressed);
    LedAction = on;
    #ifdef MY_DEBUG_SKETCH
      Serial.println("calibratie actief");
    #endif
    }
     
  switch (Action) {
    case start_windowopen:
        Action = run_windowopen;
        LedAction = fast;
      // melden aan openHab  wordt nu processButtons gedaan
      // send(msgWindowOpen.set(1), 1);    // 1 => tripped  0 => Untripped
    break;

    case run_windowopen:
      // wachten tot raam weer dicht is
      LedAction = slow;
    break;

    case powerup:      // allereerste opstart na powerup
        perc_cur = 100 ;
        perc_new = 0 ; 
        Action = run_stop;  // hier wordt uiteindelijk gestart met het inlopen bij powerup
      break;
      
    case start_up:    // start relais om scherm omhoog te laten lopen
      ShutterUp();
      break;
      
    case run_up:    // scherm loopt omhoog
    // TODO iedere 5% waarde naar OH sturen
      if ((perc_cur % 5) == 0){
        if (sendPerc != perc_cur){                    // alleen als deze waarde nog niet verstuurd is
          send(msgShutterPosition.set(perc_cur), 1);  // verstuur nieuwe stand 
          sendPerc = perc_cur;                         // onthou welke waarde er verstuurd is
        }
      }
      if (perc_cur == 0)        { Action = start_stop; }     // als einde is bereikt
      if (perc_cur == perc_new) { Action = start_stop; }     // als juiste plek is bereikt
      break;
      
    case start_dwn:    // start relais om scherm omlaag te laten lopen
      ShutterDown();
      break;
      
    case run_dwn:    // scherm loopt omlaag
      if ((perc_cur % 5) == 0){
        if (sendPerc != perc_cur){                    // alleen als deze waarde nog niet verstuurd is
          send(msgShutterPosition.set(perc_cur), 1);  // verstuur nieuwe stand 
          sendPerc = perc_cur;                         // onthou welke waarde er verstuurd is
        }
      }
      if (perc_cur == 100)      { Action = start_stop; } // als einde is bereikt
      if (perc_cur == perc_new) { Action = start_stop; } // als juiste plek is bereikt
      break;
      
    case start_stop:       // scherm loopt, stoppen
      ShutterStop();
      send(msgShutterPosition.set(perc_cur), 1);  // verstuur de stand waarop gestopt is
      break;

     case run_stop:       // scherm staat stil, kijken of er nog wat moet gebeuren
 if (perc_cur > perc_new){
        Action = start_up;
        #ifdef MY_DEBUG_SKETCH
          Serial.println(" scherm gaat inlopen ");
          Serial.print(" perc_cur ");
          Serial.println(perc_cur);
          Serial.print(" perc_new ");
          Serial.println(perc_new);          
        #endif
      }
      if (perc_cur < perc_new){
        Action = start_dwn;
        #ifdef MY_DEBUG_SKETCH
          Serial.println(" scherm gaat uitlopen ");
          Serial.print(" perc_cur ");
          Serial.println(perc_cur);
          Serial.print(" perc_new ");
          Serial.println(perc_new);          
        #endif
      }
      break;
  }
}

/* ======================================================================
Function: Button_gedrukt
Purpose : Bezig met normaal gebruik  
Comments: 
====================================================================== */
void Button_gedrukt(uint8_t button) {
  switch (button) {
      case BT_PRESS_UP:
        #ifdef MY_DEBUG_SKETCH
          Serial.println("BT_PRESS_UP gedrukt ");
        #endif
        perc_new = 0;
        send(msgUp.set(1), 1);
        send(msgShutterPosition.set(perc_cur), 1);     
        break;
      
      case BT_PRESS_DOWN:
        #ifdef MY_DEBUG_SKETCH
          Serial.println("BT_PRESS_DOWN gedrukt ");
        #endif
        perc_new = 100 ;
        send(msgDown.set(1), 1); 
        send(msgShutterPosition.set(perc_cur), 1);     
        break;
      
      case BT_PRESS_STOP:
        #ifdef MY_DEBUG_SKETCH
          Serial.println("BT_PRESS_STOP gedrukt ");
        #endif
        Action = start_stop ; 
        //send(msgStop.set(1), 1);
        break;            
    }
  }
  
/* ======================================================================
Function: Button_Calibratie
Purpose : afhandelen van calibratie 
Comments: 
====================================================================== */
void Button_Calibratie(uint8_t button) {

      switch (button) {
      case BT_PRESS_UP:
        timer.disable(timdownId);             // stop scherm down timer, voor het geval
        timer.disable(timupId);               // stop scherm up timer     

        #ifdef MY_DEBUG_SKETCH
          Serial.println("BT_PRESS_UP calibratie gedrukt ");
        #endif
        timCal = 0;                 // calibratie timer op 0 zetten
        timer.enable(timcalId);     // start calibratie timer
        digitalWrite(RELAY_UPDOWN, RELAY_UP);   // 0=RELAY_UP, 1= RELAY_DOWN
        wait(500);                              // V2.29 25-03-2019
        digitalWrite(RELAY_POWER, RELAY_ON);    // Power ON motor
        Action = cal_up;
        break;
      
      case BT_PRESS_DOWN:
        timer.disable(timdownId);             // stop scherm down timer, voor het geval
        timer.disable(timupId);               // stop scherm up timer     
        #ifdef MY_DEBUG_SKETCH
          Serial.println("BT_PRESS_DOWN calibratie gedrukt ");
        #endif
        timCal = 0;                 // calibratie timer op 0 zetten
        timer.enable(timcalId);     // start calibratie timer
        digitalWrite(RELAY_UPDOWN, RELAY_DOWN); // 0=RELAY_UP, 1= RELAY_DOWN
        wait(500);                              // V2.29 25-03-2019
        digitalWrite(RELAY_POWER, RELAY_ON);    // Power ON motor
        Action = cal_dwn;
        break;
      
      case BT_PRESS_STOP:
        timer.disable(timcalId);    // stop calibratie timer
        digitalWrite(RELAY_POWER, RELAY_OFF);    // Power OFF motor
        wait(250);                              // V2.29 25-03-2019
        digitalWrite(RELAY_UPDOWN, RELAY_OFF);  // 0=RELAY_UP, 1= RELAY_DOWN
        
        #ifdef MY_DEBUG_SKETCH
          Serial.println("BT_PRESS_STOP calibratie gedrukt ");
          Serial.print("!!!!!!!!!!!Action : "); Serial.println(Action);
        #endif
        
        if (Action == cal_up) {
          Action = cal_stop ;
          EEPROM.write(ADDR_UP, timCal);
          settimers();
          #ifdef MY_DEBUG_SKETCH
            Serial.print("Calibratie up gestopt nieuwe waarde : ");
            Serial.println(timCal);
          #endif
        }
        
        if (Action == cal_dwn){
          Action = cal_stop ;
          EEPROM.write(ADDR_DWN, timCal); 
          settimers();
          #ifdef MY_DEBUG_SKETCH
            Serial.print("Calibratie down gestopt nieuwe waarde : ");
            Serial.println(timCal);
          #endif
        }
        break;            
    }    
  }


/* ======================================================================
Function: receive
Purpose : Mysensors incomming message  
Comments: 
====================================================================== */
void receive(const MyMessage &message) {
  
  if (message.isAck()) {}
  else {
    // Message received : Open shutters
    if (message.type == V_UP && message.sensor==CHILD_ID_ROLLERSHUTTER) {
      #ifdef MY_DEBUG_SKETCH 
        Serial.println(F("CMD: Up")); 
        Serial.print(F("CMD: Up  perc_new is : ")); 
        Serial.println(perc_new);       
      #endif 
      perc_new = 0;
      send(msgShutterPosition.set(perc_cur), 1);  // verstuur huidige stand 
    }  
       
    // Message received : Close shutters
    if (message.type == V_DOWN && message.sensor==CHILD_ID_ROLLERSHUTTER) {
      #ifdef MY_DEBUG_SKETCH 
        Serial.println(F("CMD: Down")); 
        Serial.print(F("CMD: Down  perc_new is : ")); 
        Serial.println(perc_new);       
      #endif 
      perc_new = 100;
      send(msgShutterPosition.set(perc_cur), 1);  // verstuur huidige stand 
    }
  
    // Message received : Stop shutters motor
    if (message.type == V_STOP && message.sensor==CHILD_ID_ROLLERSHUTTER) {
       #ifdef MY_DEBUG_SKETCH 
        Serial.println(F("CMD: Stop")); 
       #endif 
       Action = start_stop; 
     }     
    
    // Message received : Set position of Rollershutter 
    if (message.type == V_PERCENTAGE && message.sensor==CHILD_ID_ROLLERSHUTTER) {
       if (message.getByte() > 100) perc_new= 100; else perc_new = message.getByte();
  
       #ifdef MY_DEBUG_SKETCH 
          Serial.print(F("CMD: Set position to ")); 
          // Write some debug info               
          Serial.print(message.getByte());Serial.println(F("%")); 
       #endif     
     } 

    if (message.type == V_STATUS && message.sensor==CHILD_ID_BLOCKDOWN) {
       if (message.getByte() == 1){
         BlockDown = true;
         LedAction = slow;
       }
       else{
        BlockDown = false;
        LedAction = off;
       }
     } 

    if (message.type == V_STATUS && message.sensor==CHILD_ID_GETVALUES) {
       if (message.getByte() == 1) SendValues();
     }   
  }  
}


/* ======================================================================
Function: processButtons
Purpose : Read buttons and update status
Output  : button pressed
Comments: 
====================================================================== */
uint8_t processButtons() {
 
  uint8_t result = BT_PRESS_NONE;
  
  if (debounceUp.update()) { 
    // Button Up change detected
    if (debounceUp.fell()) result = BT_PRESS_UP;               
   // else result = BT_PRESS_STOP; // Button released          
  }

  if (debounceDown.update()) {
    // Button Down change detected
    if (debounceDown.fell()) result = BT_PRESS_DOWN;
   // else result = BT_PRESS_STOP;
  }

  if (debounceStop.update()) { 
    // Button Stop change detected
    if (debounceStop.fell()) result = BT_PRESS_STOP;
  }

  if (debounceWindow.update()) {           // wijziging in raamstand gedetecteerd
    if (debounceWindow.fell()){
      send(msgWindowOpen.set(0), 1);    // raam dicht gegaan  0 => Untripped   doorgeven aan OH
      #ifdef MY_DEBUG_SKETCH
         Serial.print("Raam dicht gedaan ");
      #endif 
    }
    if (debounceWindow.rose()){
      send(msgWindowOpen.set(1), 1);    // raam open gegaan   1 => tripped    doorgeven aan OH
      #ifdef MY_DEBUG_SKETCH
        Serial.print("Raam open gedaan ShutterStop ");
      #endif
      ShutterStop(); 
    }
 }

  if (!raamdicht()) result = WINDOW_OPEN;
  
  return result;
  
}

/* ======================================================================
Function: TimUp
Purpose : Routine welke iedere 1% aangeroepen wordt als scherm omhoog loopt  
Comments: Snelheid van omhoog en omlaag lopen kan verschillen
====================================================================== */
void TimUp(){
  perc_cur--;
}

/* ======================================================================
Function: TimDown
Purpose : Routine welke iedere 1% aangeroepen wordt als scherm omlaag loopt  
Comments: Snelheid van omhoog en omlaag lopen kan verschillen
====================================================================== */
void TimDown(){
  perc_cur++;
}

/* ======================================================================
Function: TimCal
Purpose : Routine welke iedere seconde aangeroepen wordt als scherm loopt, om looptijd te bepalen  
Comments: 
====================================================================== */
void TimCal(){
  timCal++;
  #ifdef MY_DEBUG_SKETCH
    Serial.print(" !!!!!!!!!!! TimCal : ");
    Serial.println(timCal);
  #endif
}

/* ======================================================================
Function: TimLed
Purpose : Routine welke iedere seconde aangeroepen  
Comments: 
====================================================================== */
void TimLed(){

  LedAction=off;
  if(digitalRead(CALIBRATION_PIN)==LOW){ LedAction = slow; }
  if(BlockDown) { LedAction = on; }      
  if(digitalRead(WINDOW_PIN)==HIGH){ LedAction = fast;   }
  
  if (ledtel<4) ledtel++; else ledtel=0;
  switch(LedAction){
    case off:
      digitalWrite(LED, 0);
    break;

    case on:
      digitalWrite(LED, 1);
    break;

    case slow:
      if(ledtel==0) digitalWrite(LED, 1); else digitalWrite(LED, 0);
    break;

    case fast:
      if(ledtel==0 || ledtel==2) digitalWrite(LED, 1); else digitalWrite(LED, 0);
    break;
  }
}


/* ======================================================================
Function: ShutterUp
Purpose : State : Processing Up command
Comments: 
====================================================================== */
void ShutterUp() { 
  if (raamdicht()){
    digitalWrite(RELAY_UPDOWN, RELAY_UP); // 0=RELAY_UP, 1= RELAY_DOWN
    wait(250);                              // V2.29 25-03-2019
    digitalWrite(RELAY_POWER, RELAY_ON);  // Power ON motor
    timer.disable(timdownId);             // stop scherm down timer, voor het geval
    timer.enable(timupId);                // start scherm up timer 
    Action = run_up;    
  }   
}

/* ======================================================================
Function: ShutterDown
Purpose : State : Processing Down command
Comments: 
====================================================================== */
void ShutterDown() {
  if(!BlockDown){
    if (raamdicht()){   
      digitalWrite(RELAY_UPDOWN, RELAY_DOWN); // 0=RELAY_UP, 1= RELAY_DOWN 
      wait(250);                              // V2.29 25-03-2019 
      digitalWrite(RELAY_POWER, RELAY_ON);    // Power ON motor
      timer.enable(timdownId);                // start scherm down timer
      timer.disable(timupId);                 // stop scherm up timer 
      Action = run_dwn; 
    }
  }  
}

/* ======================================================================
Function: ShutterStop
Purpose : State : Processing Stop command
Comments: 
====================================================================== */
void ShutterStop() {
  #ifdef MY_DEBUG_SKETCH
    Serial.println(" !!!!!!!!!!! Shutterstop");
    Serial.print(" perc_cur ");
    Serial.println(perc_cur);
    Serial.print("perc_new ");
    Serial.println(perc_new);
  #endif
  
  digitalWrite(RELAY_POWER, RELAY_OFF);  // Power OFF motor 
  wait(250);                              // V2.29 25-03-2019
  digitalWrite(RELAY_UPDOWN, RELAY_OFF);
 
  timer.disable(timdownId); // stop scherm down timer, voor het geval
  timer.disable(timupId);     // stop scherm up timer
  
  perc_new = perc_cur;        // als via stopknop gestopt, werk perc_new bij

  Action = run_stop;
}


/* ======================================================================
Function: raamopen
Purpose : boolean windowopen  
Comments: 
====================================================================== */
bool raamdicht() {
  if (digitalRead(WINDOW_PIN) == LOW){   
    return true;   // raam dicht
  }
  else
    return false;    // raam open
}

/* ======================================================================
Function: calibratie
Purpose : boolean wel of geen calibratie 
Comments: 
====================================================================== */
bool calibratie() {
  if (digitalRead(CALIBRATION_PIN) == LOW)   
    return true;   // calibreren
  else
    return false;    // niet calibreren
}


void showstatus(){
/*
  switch (Action){
    case noinit:
      Serial.println("case noinit");
     break;
     
    case  cal_up:
      Serial.println("case  cal_up");
     break;
     
    case cal_dwn:
      Serial.println("case cal_dwn");
     break;
     
    case cal_stop:
      Serial.println("case cal_stop");
     break;
     
    case start_up:
      Serial.println("case start_up");
     break;
     
    case run_up:
      Serial.println("case run_up");
     break;
     
    case start_dwn:
      Serial.println("case start_dwn");
     break;
     
    case run_dwn:
      Serial.println("case run_dwn");
     break;
     
    case start_stop:
      Serial.println("case start_stop");
     break;
     
    case run_stop:
      Serial.println("case run_stop");
     break;
     
    case powerup:
      Serial.println("case powerup");
     break;
     
    case start_windowopen:
      Serial.println("case start_windowopen");
     break;
     
    case run_windowopen:
      Serial.println("case run_windowopen");
     break;
  }
*/
}


/* ======================================================================
Function: SendValues
Purpose : verstuurd alle bekende waarden naar OpenHab
Output  : 
Comments: dit kan gebeuren bij opstarten van OpenHab
====================================================================== */
void SendValues(){
  // verstuur lokatie van scherm / rolluik
    send(msgShutterPosition.set(perc_cur), 1);  // verstuur nieuwe stand
    wait(500);                              // V2.29 25-03-2019
    if(raamdicht()){
      send(msgWindowOpen.set(0), 1);
    }
    else{
      send(msgWindowOpen.set(1), 1);      
    }
}
