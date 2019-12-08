/* WaterBot2
 *
 * Version 2.0 of Tree Watering Robot
 *       
 * Tree watering system
 * 
 * Place supply switch at bottom of water tank
 * Place Tree switch at level you wwant watering to begin
 * Place Tree level sensor in stand's float hole
 * 
 * If tree switch is higher than level sernsor, error will be reported
 * If tree switch is lower than level sensor, pump won't run until both are satisfied,
 *    but risk of pump timing out of it runs too long, since water level is very low.  
 *    Care must be taken to not allow water level to get undesirably low.
 *    
 * LCD displays and ESP8266 are optional
 * 
 * Water Level Sensor
 * https://www.amazon.com/gp/product/B07BFPP4TQ/ref=ppx_yo_dt_b_asin_title_o01_s00?ie=UTF8&psc=1
 * Water Switch
 * https://www.amazon.com/gp/product/B0811GRVJH/ref=ppx_yo_dt_b_asin_title_o01_s00?ie=UTF8&psc=1
 * Water Pump
 * https://www.amazon.com/gp/product/B0744FWNFR/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
 * 
 * Issues: Pump needs flyback diods
 * 
 */
#include <FreqMeasure.h>
#include "TimedBlink.h"

//PIN DEFINITIONS
#define PIN_SERIAL_TX      0
#define PIN_SERIAL_RX      1

#define PIN_SPI_MOSI      11
#define PIN_SPI_MISO      12
#define PIN_SPI_SCK       13
#define PIN_SPI_SS        17

#define PIN_TREE_SENSOR   14
#define PIN_LEVEL_SENSOR   8
#define PIN_SUPPLY_SENSOR  7

#define PIN_TREE_LED       2
#define PIN_LEVEL1_LED     6
#define PIN_LEVEL2_LED     5 
#define PIN_LEVEL3_LED     4
#define PIN_LEVEL4_LED     3
#define PIN_SUPPLY_LED     9

#define PIN_PUMP          15
#define PIN_LAMP          16

//Sensor characteristics (Hz)
#define WATER_LEVEL_0 20
#define WATER_LEVEL_1 50
#define WATER_LEVEL_2 100
#define WATER_LEVEL_3 200
#define WATER_LEVEL_4 400

//States
int     SupplyState = 0;
int     TreeState = 0;
#define SENSOR_WET 0
#define SENSOR_DRY 1
#define SENSOR_ERR 2

int     PumpState = 0;
#define PUMP_OFF 0
#define PUMP_ON  1
#define PUMP_ERR 2

//Other parameters
#define AVERAGE_LENGTH 30  // Number of points to average for frequnecy calc
#define PUMP_MAXTIME   10  // Number of seconds to allow pump to run

double sum = 0;
int count = 0;
double frequency = 0;
unsigned long prev_time_freq = 0;
unsigned long prev_time_ser = 0;

//Inputs
int SupplySwitch = 0;
int TreeSwitch = 0;
unsigned long TreeLevel = 0;

// TIming state vars
unsigned long current_time = 0;
double timeout = 0;
double PumpStart = 0;

TimedBlink level1_led(PIN_LEVEL1_LED);
TimedBlink level2_led(PIN_LEVEL2_LED);
TimedBlink level3_led(PIN_LEVEL3_LED);
TimedBlink level4_led(PIN_LEVEL4_LED);
TimedBlink status_lamp(PIN_LAMP);

void setup() {

  Serial.begin(57600);
  Serial.println("Serial Initialized");

  // Intialize Inputs
  pinMode(PIN_TREE_SENSOR,INPUT);
  pinMode(PIN_LEVEL_SENSOR,INPUT);
  pinMode(PIN_SUPPLY_SENSOR,INPUT_PULLUP);

  // Intialize Outputs
  pinMode(PIN_TREE_LED,OUTPUT);
  pinMode(PIN_LEVEL1_LED,OUTPUT);
  pinMode(PIN_LEVEL2_LED,OUTPUT);
  pinMode(PIN_LEVEL3_LED,OUTPUT);
  pinMode(PIN_LEVEL4_LED,OUTPUT);
  pinMode(PIN_SUPPLY_LED,OUTPUT);
  pinMode(PIN_PUMP,OUTPUT);
  pinMode(PIN_LAMP,OUTPUT);
  level1_led.blink(50,50);
  level2_led.blink(50,50);
  level3_led.blink(50,50);
  level4_led.blink(50,50);
  
  FreqMeasure.begin();

  timeout = AVERAGE_LENGTH/(WATER_LEVEL_0*0.9);

}


void loop() {

  current_time = millis();

  ReadInputs();

  UpdateStates();

  WriteOutputs();

  UpdateDisplays();
  
  LogData();

  if ((current_time - prev_time_ser) > 2000) {

    DebugOutputs();
    prev_time_ser = current_time;
  }

}


void ReadInputs() {
  //Read Sensors
  
  SupplySwitch = digitalRead(PIN_SUPPLY_SENSOR);
  
  TreeSwitch = digitalRead(PIN_TREE_SENSOR);

  if (FreqMeasure.available()) {
    // average several reading together
    sum = sum + FreqMeasure.read();
    count = count + 1;
    if (count > AVERAGE_LENGTH) {
      // Reset timer
      prev_time_freq = current_time;
      frequency = FreqMeasure.countToFrequency(sum / count);
      sum = 0;
      count = 0;
      TreeLevel = 0;
      if (frequency > (WATER_LEVEL_0*0.9)) TreeLevel++;
      if (frequency > (WATER_LEVEL_1*0.9)) TreeLevel++;
      if (frequency > (WATER_LEVEL_2*0.9)) TreeLevel++;
      if (frequency > (WATER_LEVEL_3*0.9)) TreeLevel++;
      if (frequency > (WATER_LEVEL_4*0.9)) TreeLevel++;
    }
  }
//  if ((current_time - prev_time_freq) > timeout) {
//     TreeLevel = 0;
//     Serial.println("timeout");
//  }

}

void UpdateStates() {
  //Update States
  
  if (SupplySwitch == 0) {
    SupplyState = SENSOR_WET; // Low = Wet
  } else {
    SupplyState = SENSOR_DRY;
  }

  // Fuse TreeSwitch and TreeLevel into TreeState
  // ToDo: Add hysteresis
  if (TreeLevel == 0) {         // Sensor Timeout
    TreeState = SENSOR_ERR; 
  } else if (TreeSwitch == 0) { // Low = Wet (regardless of level sensor)
    TreeState = SENSOR_WET; 
  } else if (TreeLevel < 5) {  // Sensor Less than full and Switch = Dry
    TreeState = SENSOR_DRY; 
  } else {
    TreeState = SENSOR_ERR;   // Sensor Full & Switch Dry conflict -- Bad switch placement?
  }
  // Fuse TreeState and SwitchState into PumpState
  switch (PumpState) {
    case PUMP_OFF :
      if ((TreeState == SENSOR_DRY) && (SupplyState == SENSOR_WET)) {
        PumpStart = current_time; // Record pumping start time
        PumpState = PUMP_ON;
      }
      break;
    
    case PUMP_ON :
      // If TreeState or Supply State change, stop pumping
      if ((TreeState != SENSOR_DRY) || (SupplyState != SENSOR_WET)) {
        PumpState = PUMP_OFF;
      }
      
      if ((current_time - PumpStart) > PUMP_MAXTIME*1000) {
        PumpState = PUMP_ERR;
      }
      break;

    case PUMP_ERR :
      // Do nothing until power cycle
      // Todo: Add auto-reset sequence? Allow pumping as long as positive level progress being made?
    break;
  }
}

void WriteOutputs() {
  // Update Pump output
  if (PumpState == PUMP_ON) {
    digitalWrite(PIN_PUMP,false);
  } else {
    digitalWrite(PIN_PUMP,true);
  }

  // Update remote status lamp (in order of importance)
  // Note relay is active low, so "on" is really "off" in the blink routine
  //
  // Fast Blink = Sensor Error (Either Level=0Hz or Switch Dry while Level OK)
  // On Blip = Pump Lockout (Pump ran too long)
  // Off Blip = Supply Empty
  // Slow Blink = Pump On
  // On = Standby
  if (TreeState == SENSOR_ERR) {
    status_lamp.blink(50,50); 
  } else if (PumpState == PUMP_ERR) {
    status_lamp.blink(2000,50);
  } else if (SupplyState == SENSOR_DRY) {
    status_lamp.blink(50,2000);
  } else if (PumpState == PUMP_ON) {
    status_lamp.blink(1000,1000);
  } else {
    status_lamp.on();
  }
  
  // Update LEDs
  if (SupplySwitch == SENSOR_WET) {
    digitalWrite(PIN_SUPPLY_LED,true);
  } else {
    digitalWrite(PIN_SUPPLY_LED,false);
  }
  
  if (TreeSwitch == SENSOR_WET) {
    digitalWrite(PIN_TREE_LED,true);
  } else {
    digitalWrite(PIN_TREE_LED,false);
  }
  
  switch (TreeLevel) {
    case 0:
      level1_led.blink();
      level2_led.blink();
      level3_led.blink();
      level4_led.blink();
      break;
    case 1:
      level1_led.off();
      level2_led.off();
      level3_led.off();
      level4_led.off();
      break;
    case 2:
      level1_led.off();
      level2_led.off();
      level3_led.off();
      level4_led.on();
      break;
    case 3:
      level1_led.off();
      level2_led.off();
      level3_led.on();
      level4_led.on();
      break;
    case 4:
      level1_led.off();
      level2_led.on();
      level3_led.on();
      level4_led.on();
      break;
    case 5:
      level1_led.on();
      level2_led.on();
      level3_led.on();
      level4_led.on();
      break;
  }

}

void UpdateDisplays() {
  //ToDo: Drive I2C display directly -OR- Send to ESP8266 via UART
  //ToDo: Send SPI LCD commands (Alternative to lamp... 2020 enhancement)
}

void LogData() {
  //ToDo: Send data to ESP8266 for MQTDD logging (2020 enhancement)
}

void DebugOutputs() {

  Serial.println("==INPUTS==");
  Serial.print(" SupplySwitch = ");
  switch (SupplySwitch) {
    case SENSOR_WET : 
    Serial.println("WET");
    break;

    case SENSOR_DRY : 
    Serial.println("DRY");
    break;
    
    case SENSOR_ERR : 
    Serial.println("ERROR");
    break;
  }
  Serial.print(" TreeSwitch = ");
  switch (TreeSwitch) {
   case SENSOR_WET : 
    Serial.println("WET");
    break;

    case SENSOR_DRY : 
    Serial.println("DRY");
    break;
    
    case SENSOR_ERR : 
    Serial.println("ERROR");
    break;
  }
  Serial.print(" TreeLevel = ");
  Serial.print(TreeLevel);
  Serial.print(" (");
  Serial.print(frequency);
  Serial.println("Hz)");

  Serial.println("==STATES==");
  Serial.print(" SupplyState = ");
  switch (SupplyState) {
    case SENSOR_WET : 
    Serial.println("WET");
    break;

    case SENSOR_DRY : 
    Serial.println("DRY");
    break;
    
    case SENSOR_ERR : 
    Serial.println("ERROR");
    break;
  }
  Serial.print(" TreeState = ");
  switch (TreeState) {
    case SENSOR_WET : 
    Serial.println("WET");
    break;

    case SENSOR_DRY : 
    Serial.println("DRY");
    break;
    
    case SENSOR_ERR : 
    Serial.println("ERROR");
    break;
  }
  Serial.print(" PumpState = ");
  switch (PumpState) {
    case PUMP_OFF : 
    Serial.println("OFF");
    break;

    case PUMP_ON : 
    Serial.print("ON (");
    Serial.print((current_time-PumpStart)/1000);
    Serial.println("s)");
    break;
    
    case PUMP_ERR : 
    Serial.println("ERROR");
    break;
  }
  
  Serial.println("");
  
}
