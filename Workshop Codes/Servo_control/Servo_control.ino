#include <Servo.h>

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position

void setup() {
  myservo.attach(3);  // attaches the servo on pin 9 to the servo object
}

void loop() {
    pos = 0;				// try experimenting with this value and rerun the code
    myservo.write(pos);              // tell servo to go to position in pos
}

