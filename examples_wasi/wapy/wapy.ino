#include <Arduino.h>

#define False false
#define True true
#define STRINGIFY(a) #a
#define TOSTRING(x) STRINGIFY(x)


void setup() {
    Serial.begin(115200);
    while (!Serial) { yield(); }
    delay(333);
    Serial.println("\r\n");
    Serial.print(__FILE__);
    Serial.println(":");
}

void loop() {
}
