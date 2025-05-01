#include <Arduino.h>

// put function declarations here:
void adcTest();

void setup() {
  Serial.begin(115200);
  delay(1000);
  adcTest();
}

void loop() {
  while(true);
}
