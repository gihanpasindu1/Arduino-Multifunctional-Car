#include <Servo.h>
#include <NewPing.h>

const int LeftMotorForward  = 7;  
const int LeftMotorBackward = 6;  
const int RightMotorForward = 4;  
const int RightMotorBackward= 5;  

#define trig_pin A1
#define echo_pin A2
#define maximum_distance 200
NewPing sonar(trig_pin, echo_pin, maximum_distance);

Servo servo_motor;

#define cliff_pin A3
int cliffThreshold = 100;

#define frontLight 11
#define backLight 12
bool frontLightOn = false;
bool backLightOn  = false;

#define redLED 8
#define blueLED 9
bool vipActive = false;
unsigned long vipStartTime = 0;

int mode = 0;
int distance = 100;
boolean goesForward = false;

int motorSpeed = 150;

unsigned long hornStartTime = 0;
int hornStep = 0;
bool hornActive = false;

void setup() {
  Serial.begin(9600);

  pinMode(LeftMotorForward, OUTPUT);
  pinMode(LeftMotorBackward, OUTPUT);
  pinMode(RightMotorForward, OUTPUT);
  pinMode(RightMotorBackward, OUTPUT);
  pinMode(13, OUTPUT);

  pinMode(cliff_pin, INPUT);
  pinMode(frontLight, OUTPUT);
  pinMode(backLight, OUTPUT);

  pinMode(redLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  servo_motor.attach(10);
  servo_motor.write(115);
  delay(1000);

  Serial.println("Car is in IDLE mode.");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    Serial.print("Signal received: "); Serial.println(c);
    handleCommand(c);
  }

  updateReversHorn();

  if (vipActive) {
    runVIPPattern();
    if (millis() - vipStartTime >= 5000) { 
      vipActive = false;
      digitalWrite(redLED, LOW);
      digitalWrite(blueLED, LOW);
      Serial.println("VIP OFF (auto)");
    }
    return;
  }

  distance = readPing();
  int cliffValue = analogRead(cliff_pin);  
  boolean cliffDetected = (cliffValue > cliffThreshold);  

  if (cliffDetected) {
    moveStop();
    Serial.print("Cliff detected! Analog value: ");
    Serial.println(cliffValue);
    return;
  }

  if (mode == 2 && goesForward && distance <= 25) {
    moveStop();
    Serial.println("Obstacle detected in manual mode! Stopping.");
  }

  if (mode == 1) {
    autoMode();
  }
}

void handleCommand(char c) {
  if (c == 'I') { mode = 0; Serial.println("Mode: IDLE"); moveStop(); }
  if (c == 'A') { mode = 1; Serial.println("Mode: AUTO"); }
  if (c == 'M') { mode = 2; Serial.println("Mode: MANUAL"); }

  if (mode == 2) {
    if (c == 'F') { 
      int cliffValue = analogRead(cliff_pin);
      if (cliffValue <= cliffThreshold && distance > 25) moveForward(); 
      Serial.println("Manual: Forward"); 
    }
    if (c == 'B') { 
      moveBackward(); 
      startReversHorn();
      Serial.println("Manual: Backward"); 
    }
    if (c == 'L') { turnLeftManual(); Serial.println("Manual: Left"); }
    if (c == 'R') { turnRightManual(); Serial.println("Manual: Right"); }
    if (c == 'S') { moveStop(); Serial.println("Manual: Stop"); }
    if (c == 'H') { horn();}
  }

  if (c == 'X') {
    frontLightOn = !frontLightOn;
    digitalWrite(frontLight, frontLightOn);
    Serial.print("Front Light: "); Serial.println(frontLightOn ? "ON" : "OFF");
  }
  if (c == 'Y') {
    backLightOn = !backLightOn;
    digitalWrite(backLight, backLightOn);
    Serial.print("Back Light: "); Serial.println(backLightOn ? "ON" : "OFF");
  }

  if (c == 'V') {
    vipActive = true;
    vipStartTime = millis();
    Serial.println("VIP ON");
  }
}

void runVIPPattern() {
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, HIGH);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, HIGH);
  delay(75);
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, HIGH);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);
  delay(75);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, HIGH);
  delay(75);
}

void autoMode() {
  int distanceRight = 0;
  int distanceLeft = 0;

  if (distance <= 45) {
    moveStop();
    delay(300);
    moveBackward();
    delay(400);
    moveStop();
    delay(300);

    distanceRight = lookRight();
    delay(300);
    distanceLeft = lookLeft();
    delay(300);

    if (distanceRight >= distanceLeft) {
      turnRight();
      Serial.println("Auto: Turning Right");
    } else {
      turnLeft();
      Serial.println("Auto: Turning Left");
    }
  } else {
    moveForward();
    Serial.println("Auto: Moving Forward");
  }
}

int lookRight() {
  servo_motor.write(50);
  delay(500);
  int d = readPing();
  servo_motor.write(115);
  return d;
}

int lookLeft() {
  servo_motor.write(170);
  delay(500);
  int d = readPing();
  servo_motor.write(115);
  return d;
}

int readPing() {
  delay(50);
  int cm = sonar.ping_cm();
  if (cm == 0) cm = 250;
  return cm;
}

void moveStop() {
  analogWrite(LeftMotorForward, 0);
  analogWrite(RightMotorForward, 0);
  analogWrite(LeftMotorBackward, 0);
  analogWrite(RightMotorBackward, 0);
  goesForward = false;
}

void moveForward() {
  analogWrite(LeftMotorForward, motorSpeed);
  analogWrite(RightMotorForward, motorSpeed);
  analogWrite(LeftMotorBackward, 0);
  analogWrite(RightMotorBackward, 0);
  goesForward = true;
}

void moveBackward() {
  analogWrite(LeftMotorBackward, motorSpeed);
  analogWrite(RightMotorBackward, motorSpeed);
  analogWrite(LeftMotorForward, 0);
  analogWrite(RightMotorForward, 0);
  goesForward = false;
}

void turnRight() {
  analogWrite(LeftMotorForward, motorSpeed);
  analogWrite(RightMotorBackward, motorSpeed);
  analogWrite(LeftMotorBackward, 0);
  analogWrite(RightMotorForward, 0);
  delay(250);
  moveForward();
}

void turnLeft() {
  analogWrite(LeftMotorBackward, motorSpeed);
  analogWrite(RightMotorForward, motorSpeed);
  analogWrite(LeftMotorForward, 0);
  analogWrite(RightMotorBackward, 0);
  delay(250);
  moveForward();
}

void turnRightManual() {
  analogWrite(LeftMotorForward, motorSpeed);
  analogWrite(RightMotorBackward, motorSpeed);
  analogWrite(LeftMotorBackward, 0);
  analogWrite(RightMotorForward, 0);
}

void turnLeftManual() {
  analogWrite(LeftMotorBackward, motorSpeed);
  analogWrite(RightMotorForward, motorSpeed);
  analogWrite(LeftMotorForward, 0);
  analogWrite(RightMotorBackward, 0);
}

void startReversHorn() {
  hornStartTime = millis();
  hornStep = 0;
  hornActive = true;
}

void updateReversHorn() {
  if (!hornActive) return;

  unsigned long now = millis();

  switch(hornStep) {
    case 0:
      digitalWrite(13, HIGH);
      hornStartTime = now;
      hornStep = 1;
      break;
    case 1:
      if (now - hornStartTime >= 600) {
        digitalWrite(13, LOW);
        hornStartTime = now;
        hornStep = 2;
      }
      break;
    case 2:
      if (now - hornStartTime >= 600) {
        digitalWrite(13, HIGH);
        hornStartTime = now;
        hornStep = 3;
      }
      break;
    case 3:
      if (now - hornStartTime >= 600) {
        digitalWrite(13, LOW);
        hornActive = false;
      }
      break;
  }
}

void horn() {
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  
  digitalWrite(13, HIGH);
  delay(300);
  digitalWrite(13, LOW);
  delay(100);
  
  for(int i = 0; i < 2; i++) {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(100);
  }
}