/* FreqMeasure - Example with serial output
 * http://www.pjrc.com/teensy/td_libs_FreqMeasure.html
 *
 * This example code is in the public domain.
 */
#include <FreqMeasure.h>

#define WATER_LEVEL_0 20
#define WATER_LEVEL_1 50
#define WATER_LEVEL_2 100
#define WATER_LEVEL_3 200
#define WATER_LEVEL_4 400

//PIN DEFINITIONS
#define LED_LEVEL1 2
#define LED_LEVEL2 3 
#define LED_LEVEL3 4
#define LED_LEVEL4 5
#define SENSOR_LEVEL 8

void setup() {
  Serial.begin(57600);
  FreqMeasure.begin();
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(LED_LEVEL1,OUTPUT);
  pinMode(LED_LEVEL2,OUTPUT);
  pinMode(LED_LEVEL3,OUTPUT);
  pinMode(LED_LEVEL4,OUTPUT);
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
