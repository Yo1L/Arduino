#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <MeMCore.h>
#define MOTOR_STOP  0
#define MOTOR_MOVE_FORWARD 1
#define MOTOR_MOVE_BACKWARD 2
#define MOTOR_MOVE_LEFT 3
#define MOTOR_MOVE_RIGHT 4

MeIR ir;
MeRGBLed rgbled_7(7, 7==7?2:4);
MBotDCMotor motor(0);
MeLineFollower lineFollower(PORT_2);
int go = MOTOR_STOP,
    last_go = MOTOR_STOP;
int speed = 100;


void setup() {
  Serial.begin(9600);
  Serial.print("setup");
  ir.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  char code = ir.getCode();
  if( code ) {
    Serial.print("IR Code : ");
    Serial.println((int)code);
  }
  
  uint8_t state = lineFollower.readSensors();
  switch( state ) {
    case S1_IN_S2_IN:
      go = MOTOR_MOVE_FORWARD;
      speed = 80;
      break;
    case S1_IN_S2_OUT:
      go = MOTOR_MOVE_LEFT;
      speed = 50;
      break;
    case S1_OUT_S2_IN:
      go = MOTOR_MOVE_RIGHT;
      speed = 50;
      break;
    case S1_OUT_S2_OUT:
      go = MOTOR_STOP;
      speed = 0;
      break;
  }

  if( last_go != go ) {
    if( go != MOTOR_STOP ) {
      rgbled_7.setColor(0, 0, 235, 0);
      rgbled_7.show();
    }
    else {
      rgbled_7.setColor(0, 240, 10, 10);
      rgbled_7.show();

      switch( last_go ) {
        case MOTOR_MOVE_FORWARD:
          go = MOTOR_MOVE_BACKWARD;
          speed = 180;
          break;
        case MOTOR_MOVE_LEFT:
          go = MOTOR_MOVE_RIGHT;
          speed = 180;
          break;
        case MOTOR_MOVE_RIGHT:
          go = MOTOR_MOVE_LEFT;
          speed = 180;
          break;
      }
    }
    
    motor.move(go, speed);
    last_go = go;

    delay(100);
  }
}
