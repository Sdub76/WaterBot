/* WaterBot2
 *
 * Version 2.0 of Tree Watering Robot
 * ToDo: Add SPI display commands
 *       Add RX/TX to ESP8266
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

#define PIN_TREE_SENSOR    7
#define PIN_LEVEL_SENSOR   8
#define PIN_SUPPLY_SENSOR 14

#define PIN_TREE_LED       2
#define PIN_LEVEL1_LED     3
#define PIN_LEVEL2_LED     4 
#define PIN_LEVEL3_LED     5
#define PIN_LEVEL4_LED     6
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
#define SENSOR_WET   0
#define SENSOR_DRY   1
#define SENSOR_ERROR 2
#define PUMP_OFF 0
#define PUMP_ON  1
#define PUMP_ERR 2

//Other parameters
#define AVERAGE_LENGTH 30  // Number of points to average for frequnecy calc
#define PUMP_MAXTIME   10 // Number of seconds to allow pump to run

double sum = 0;
int count = 0;
double frequency = 0;
unsigned long prev_time_freq = 0;

unsigned long TreeLevel = 0;
boolean TreeSwitch = 0;
boolean TreeState = 0;

boolean SupplySwitch = 0;
boolean SupplyState = 0;

boolean PumpState = 0;
double PumpStart = 0;

unsigned long current_time = 0;
unsigned long freq_prev_time = 0;
double timeout = 0;

TimedBlink level1_led(PIN_LEVEL1_LED);
TimedBlink level2_led(PIN_LEVEL2_LED);
TimedBlink level3_led(PIN_LEVEL3_LED);
TimedBlink level4_led(PIN_LEVEL4_LED);
TimedBlink status_lamp(PIN_LAMP);

void setup() {

  //Serial.begin(57600);
  
  // Intialize Inputs
  pinMode(PIN_TREE_SENSOR,INPUT);
  pinMode(PIN_LEVEL_SENSOR,INPUT);
  pinMode(PIN_SUPPLY_SENSOR,INPUT);

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
      freq_prev_time = current_time;
      double frequency = FreqMeasure.countToFrequency(sum / count);
      Serial.println(frequency);
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
  if ((current_time - freq_prev_time) > timeout) {
     TreeLevel = 0;
  }


}

void UpdateStates() {
  //Update States
  
  if (SupplySwitch == 0) {
    SupplyState = SENSOR_WET; // Low = Wet
  } else {
    SupplyState = SENSOR_DRY;
  }

  if (TreeLevel == 0) {
    TreeState = SENSOR_ERROR; // Sensor Timeout
  } else if (TreeSwitch == 0) {
    TreeState = SENSOR_WET; // Low = Wet
  } else if (TreeSwitch < 5) {
    TreeState = SENSOR_DRY; // Switch Dry & Sensor Dry
  } else {
    TreeState = SENSOR_ERROR; // Sensor Wet & Switch Dry conflict
  }
  
  switch (PumpState) {
    case PUMP_OFF :
      if ((TreeState == SENSOR_DRY) && (SupplyState == SENSOR_WET)) {
        PumpStart = current_time; // Record pumping start time
        PumpState = PUMP_ON;
      }
      break;
    
    case PUMP_ON :
      // If TreeState or Supply State change, stop pumping
      if ((TreeState != SENSOR_DRY) && (SupplyState != SENSOR_WET)) {
        PumpState = PUMP_OFF;
      }
      
      if ((current_time - PumpStart) > PUMP_MAXTIME*1000) {
        PumpState = PUMP_OFF;
      }
      break;
  }
}

void WriteOutputs() {
  // Update Pump output
  if (PumpState == PUMP_ON) {
    digitalWrite(PIN_PUMP,true);
  } else {
    digitalWrite(PIN_PUMP,false);
  }

  // Update remote status lamp (in order of importance)
  // Fast Blink = Sensor Error (Either Level=0Hz or Switch Dry while Level OK)
  // Fast Blip = Supply Empty
  // Slow Blink = Pump On
  // On = Standby
  if (TreeState == SENSOR_ERROR) {
    status_lamp.blink(50,50); 
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
