#include <Servo.h>

Servo myservo;

#define trigPin 9
#define echoPin 8
#define ledpin 11
int duration, distance;
int max_distance = 40;

float normalized_angle(float distance) {
    float a = (distance / max_distance) * 180;

    if (a > 180) {
      a = 180;
    }

    return a;
}

void setup() {
//Ultrasonic Setup
pinMode(trigPin, OUTPUT);  
pinMode(echoPin, INPUT);  
pinMode(ledpin, OUTPUT);

// Initialise servo
myservo.attach(3);
myservo.write(0);
delay(1000);

//Serial Monitor Setup
Serial.begin(9600);  

}

void loop() {
  
  //Send a LOW signal through the trig Pin then delay for some microseconds
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  //Send a HIGH signal through the trig Pin then delay for some microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);  
  //Send a LOW signal through the trig Pin
  digitalWrite(trigPin, LOW);


  //Calculating distance
//Use pulseIn on the Echo Pin
 unsigned long duration = pulseIn(echoPin, 1, 30000);
 //Convert duration to distance
  float distance = 0.0343*duration/2; //distance in cm

float angle = normalized_angle(distance);
myservo.write(angle);

  //Print to Serial Monitor
  Serial.print("Distance: ");  
  Serial.println(distance);

delay(1000);
}
