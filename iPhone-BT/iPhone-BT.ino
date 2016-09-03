#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <MeMCore.h>

MeRGBLed rgbled_7(7, 7==7?2:4);
MeUltrasonicSensor ultrasonic_3(3);
MeLightSensor lightsensor(6);
MeBluetooth bt(PORT_5);
MeBuzzer buzzer;
MBotDCMotor motor(0);
MeIR ir;

int lastDirection;
int direction;
bool run = false;
long lastRun = 0;
int speed;
int distance;

#define BLUETOOTH_COMMAND_SIZE  2
#define REFRESH_COMMAND_DELAY 100 // MSECS
#define MOTOR_STOP  0
#define MOTOR_MOVE_FORWARD 1
#define MOTOR_MOVE_BACKWARD 2
#define MOTOR_MOVE_LEFT 3
#define MOTOR_MOVE_RIGHT 4

#define BUFFSIZE  4

#define BT_RUN          0x1
#define BT_SPEED        0x2
#define BT_DIRECTION    0x4
#define BT_RO_DISTANCE  0x8

template<class T> T clamp(T value, T min, T max) {
  if( value < min ) {
    return min;
  }
  else if( value > max ) {
    return max;
  }
  return value;
}

void writeBluetoothString(const char *data ) {
  if( !data ) return;
  for( size_t i=0; data[i] != '\0'; i++ ) {
    bt.write(data[i]);
  }
  bt.write('\n');
}

void sendBluetoothCommand(uint8_t order, int value) {
  bt.write(order);
  bt.write(0xff & value);
}

void toggleRun(bool isRunning) {
  run = isRunning;

  if( !run ) {
    buzzer.tone(464, 250);
    motor.move(0,0);
    rgbled_7.setColor(0,0,0,0);
    rgbled_7.show();
    delay(1000);
  }
}

uint8_t btBuffer[BUFFSIZE];
size_t bufferPosition = 0;

void resetBluetoothBuffer() {
  memset(btBuffer, 0, sizeof(char)*BUFFSIZE);
  bufferPosition = 0;
}
/**
 * Read Bluetooth command
 */
void readBluetoothCommand() {  
  for( ; bt.available() && bufferPosition<(BUFFSIZE-1); bufferPosition++ ) {
    btBuffer[bufferPosition] = bt.read();

    // 2 bytes
    // 1st - <128 : write command else >=128 Write command
    // 2nd - Value
    if( bufferPosition == 1 /*|| btBuffer[bufferPosition] == '\n' && bufferPosition>0*/ ) {
      parseBluetoothCommand(btBuffer, bufferPosition+1);
      resetBluetoothBuffer();
      return;
    }
  }

  if( bufferPosition>=BUFFSIZE ) {
    resetBluetoothBuffer();
  }
}

void parseBluetoothCommand(char *buff, size_t size) {
  /*writeBluetoothString(commands[0]);
  writeBluetoothString(commands[1]);*/

  // convert unsigned to signed int
  bool write = buff[0] & 0x80 ? true : false;
  int cmd   = 0xff & (buff[0] & ~0x80);
  int value = 0xff & buff[1];

  switch(cmd) {
    case BT_RUN: 
      {
        /**
         * run:[0|1]
         */
        if( write ) {
          toggleRun(value == 0 ? false : true);
        }
        value = run ? 1 : 0;
      }
      break;

    case BT_SPEED: 
      {
        /**
         * speed:[0-255]
         */
        if( write ) {
          speed = clamp(value, 0, 255);
        }
        value = speed; // send back the right value
      }
      break;

    case BT_DIRECTION:
      {
        /**
         * direction: [0-4]
         */
        if( write ) {
          direction = clamp(value, 0, 4);
        }
        value = direction; // send back the right value
      }
      break;

    case BT_RO_DISTANCE:
      {
        /**
         * READ ONLY Distance from a wall
         */
        value = distance > 255 ? 255 : distance;
      }
      break;
      
    default:
      // error
      cmd = 0xff;
      value = 0xff;
      break;
  }

  sendBluetoothCommand(cmd, value);
}

void setup(){
  rgbled_7.setColor(0,0,0,0);
  rgbled_7.show();

  bt.begin(115200);
  ir.begin();

  pinMode(A7, INPUT);

  direction = MOTOR_MOVE_FORWARD;

  Serial.begin(115200);

  lastRun = millis();

  lastDirection = direction = MOTOR_STOP;
  speed = 0;
  distance = 255;

  resetBluetoothBuffer();
}

void loop() {
  /**
   * Parse BT Commands
   */
  readBluetoothCommand();
  
  if( analogRead(A7) < 10 || ir.keyPressed(21) ) { // settings
    // button is pressed
    toggleRun(!run);
    return;
  }

  if( ir.keyPressed(69) ) { /* A */
    rgbled_7.setColor(0, 0, 100, 0);
    rgbled_7.show();
    speed = 80;
  }
  else if( ir.keyPressed(70) ) { /* B */
    rgbled_7.setColor(0, 220, 30, 10);
    rgbled_7.show();
    speed = 160;
  }
  else if( ir.keyPressed(71) ) { /* C */
    rgbled_7.setColor(0, 200, 0, 0);
    rgbled_7.show();
    speed = 255;
  }

  if( ir.keyPressed(64) ) {
    direction = MOTOR_MOVE_FORWARD;
  }
  else if( ir.keyPressed(25) ) {
    direction = MOTOR_MOVE_BACKWARD;
  }
  else if( ir.keyPressed(7) ) {
    direction = MOTOR_MOVE_LEFT;
  }
  else if( ir.keyPressed(9) ) {
    direction = MOTOR_MOVE_RIGHT;
  }

  // avoid sleeping (delay)
  long now = millis();
  if( run && ((lastRun + REFRESH_COMMAND_DELAY) < now) ) {
    lastRun = now;

    distance = ultrasonic_3.distanceCm();
    int go = direction;
    
    if( go == MOTOR_MOVE_FORWARD && distance < 30 ) {
      // send alert info
      sendBluetoothCommand(BT_RO_DISTANCE, distance);
      
      if( distance < 15 ) {
        go = MOTOR_MOVE_BACKWARD;
      }
      else {
        //go = rand() % 2 ? MOTOR_MOVE_LEFT : MOTOR_MOVE_RIGHT;
        go = MOTOR_MOVE_LEFT;
      }
      buzzer.tone(262, 250);
      rgbled_7.setColor(0,150,0,0);
    }
    else {
      rgbled_7.setColor(0,0,100,0);
      go = direction;
    }

    if( go != direction ) {
      // send info
      sendBluetoothCommand(BT_DIRECTION, direction);
    }
    
    motor.move(go, speed);
    rgbled_7.show();
  }
  
}

