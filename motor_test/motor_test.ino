// Motor control pins
const int enA = 3;  // Enable Motor A (PWM)
const int in1 = 7;  // Input 1 Motor A
const int in2 = 8;  // Input 2 Motor A

const int enB = 5;  // Enable Motor B (PWM)
const int in3 = 9;  // Input 1 Motor B
const int in4 = 10; // Input 2 Motor B

void setup() {
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  stopMotors();
}

// Helper function to stop both motors cleanly
void stopMotors() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(enA, 0);

  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enB, 0);
}

// Moves both motors forward; duration in floating-point seconds
void forward(float duration_sec) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, 128);

  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enB, 128);

  // UL multiplier prevents integer overflow on 16-bit boards
  delay((unsigned long)(duration_sec * 1000UL)); 
  stopMotors();
}

// Point-turn Left: Motor A (Left) REVERSE, Motor B (Right) FORWARD
void left(unsigned long duration_ms = 400) {
  // Motor A: Reverse
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  analogWrite(enA, 128);

  // Motor B: Forward
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enB, 128);

  delay(duration_ms);
  stopMotors();
}

// Point-turn Right: Motor A (Left) FORWARD, Motor B (Right) REVERSE
void right(unsigned long duration_ms = 400) {
  // Motor A: Forward
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, 128);

  // Motor B: Reverse
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enB, 128);

  delay(duration_ms);
  stopMotors();
}

void loop() {
  
  forward(1.0);   // Move forward for 2 seconds
  delay(3000);    // Pause 1 second


  
  left(550);     // Turn left for 1 second
  delay(3000);    // Pause 1 second
  

  
  right(550);    // Turn right for 1 second
  delay(3000);    // Pause 3 seconds before restarting the loop
  
   
}