/* FreqMeasure - Example with serial output
 * http://www.pjrc.com/teensy/td_libs_FreqMeasure.html
 *
 * This example code is in the public domain.
 */
#include <FreqMeasure.h>

#define togglepin 9

void setup() {
  Serial.begin(57600);
  FreqMeasure.begin();
  pinMode(9,OUTPUT);
}

double sum=0;
int count=0;
boolean toggle = 0;
double blinkcount = 0;

void loop() {
  if (FreqMeasure.available()) {
    // average several reading together
    sum = sum + FreqMeasure.read();
    count = count + 1;
    if (count > 30) {
      float frequency = FreqMeasure.countToFrequency(sum / count);
      Serial.println(frequency);
      sum = 0;
      count = 0;
      toggle = !toggle;
    }
  }
  digitalWrite(togglepin,toggle);
}
