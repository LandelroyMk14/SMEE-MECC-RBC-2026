// Define the motor control pins for Motor A
const int enA = 3;  // Enable pin for motor A (PWM)
const int in1 = 7;  // Input 1 for motor A
const int in2 = 8;  // Input 2 for motor A

// Define the motor control pins for Motor B
const int enB = 5;  // Enable pin for motor B (PWM)
const int in3 = 9;  // Input 1 for motor B
const int in4 = 10;  // Input 2 for motor B

void setup() {
  // Set all the motor control pins to output for Motor A
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  // Set all the motor control pins to output for Motor B
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // Initial motor state (Stopped)
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(enA, 0);  // Speed 0 (stopped)
 digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enB, 0);  // Speed 0 (stopped)
}

void loop() {
  // Test Motor A: Rotate forward at full speed
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, 255);  // Full speed

  delay(2000);  // Run for 2 seconds

  // Test Motor A: Stop
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(enA, 0);  // Stop

  delay(1000);  // Stop for 1 second

  // Test Motor A: Rotate backward at half speed
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  analogWrite(enA, 128);  // Half speed

  delay(2000);  // Run for 2 seconds

  // Test Motor A: Stop
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(enA, 0);  // Stop

  delay(1000);  // Stop for 1 second
  // Test Motor B: Rotate forward at full speed
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enB, 255);  // Full speed

  delay(2000);  // Run for 2 seconds

  // Test Motor B: Stop
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enB, 0);  // Stop

  delay(1000);  // Stop for 1 second

  // Test Motor B: Rotate backward at half speed
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enB, 128);  // Half speed

  delay(2000);  // Run for 2 seconds

  // Test Motor B: Stop
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enB, 0);  // Stop

  delay(1000);  // Stop for 1 second
}
