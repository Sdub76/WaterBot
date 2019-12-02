/* WaterBot2
 *
 * Version 2.0 of Tree Watering Robot
 */
#include <FreqMeasure.h>

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

#define WATER_LEVEL_0 20
#define WATER_LEVEL_1 50
#define WATER_LEVEL_2 100
#define WATER_LEVEL_3 200
#define WATER_LEVEL_4 400


void setup() {
  Serial.begin(57600);
  FreqMeasure.begin();
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
}

double sum=0;
int count=0;
boolean toggle = 0;
float frequency = 0;


void loop() {

  
  if (FreqMeasure.available()) {
    // average several reading together
    sum = sum + FreqMeasure.read();
    count = count + 1;
    if (count > 30) {
      frequency = FreqMeasure.countToFrequency(sum / count);
      Serial.println(frequency);
      sum = 0;
      count = 0;
      toggle = !toggle;
    }
    digitalWrite(LED_BUILTIN,toggle);
    if (frequency > (WATER_LEVEL_1*0.9)){
      digitalWrite(LED_LEVEL1,true);
      
    } else {
      digitalWrite(LED_LEVEL1,false);
    }
    if (frequency > (WATER_LEVEL_2*0.9)){
      digitalWrite(LED_LEVEL2,true);
    } else {
      digitalWrite(LED_LEVEL2,false);
    }
    if (frequency > (WATER_LEVEL_3*0.9)){
      digitalWrite(LED_LEVEL3,true);
    } else {
      digitalWrite(LED_LEVEL3,false);
    }
    if (frequency > (WATER_LEVEL_4*0.9)){
      digitalWrite(LED_LEVEL4,true);
    } else {
      digitalWrite(LED_LEVEL4,false);
    }
  }
}

void ReadSensors() {
  
  SupplyState = digitalRead(SUPPLY_LEVEL);
  TreeState_raw = digitalRead(TREE_LEVEL);
  TreeLevel_raw = analogRead(TREE_LEVEL_A);
  
  switch (TreeState) {
    case SENSOR_DRY :
      if (TreeState_raw == SENSOR_WET) {
        // If transitioning from "dry" to "wet", add pump on-time to total watering time
        WaterTime = WaterTime + (float)(PumpTime)/1000.0;
        // Set flag to reinitialize pump timer
        NewPumpSession = true;
        TreeState = SENSOR_WET;
      }
      // Invalidate Tree Level State if raw value > threshold
      if (TreeLevel_raw > SENS_RAW_ERR) {
        // If transitioning from "dry" to "error", add pump on-time to total watering time
        WaterTime = WaterTime + (float)PumpTime/1000.0;
        // Set flag to reinitialize pump timer
        NewPumpSession = true;
        TreeState = SENSOR_ERROR;
      }
      break;
    case SENSOR_WET :
      TreeState = SENSOR_WET;
      if (TreeState_raw == SENSOR_DRY) {
        TreeState = SENSOR_DRY;
      }
      // Invalidate Tree Level State if raw value > threshold
      if (TreeLevel_raw > SENS_RAW_ERR) {
        TreeState = SENSOR_ERROR;
      }
      break;
    case SENSOR_ERROR :
      TreeState = SENSOR_WET;
      break;
  }
}
  
