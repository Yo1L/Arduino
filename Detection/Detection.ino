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
bool manual = false;
long lastRun = 0;
int speed = 80;

#define MOTOR_STOP  0
#define MOTOR_MOVE_FORWARD 1
#define MOTOR_MOVE_BACKWARD 2
#define MOTOR_MOVE_LEFT 3
#define MOTOR_MOVE_RIGHT 4


void setup(){
  rgbled_7.setColor(0,0,0,0);
  rgbled_7.show();

  bt.begin(115200);
  ir.begin();

  pinMode(A7, INPUT);

  direction = MOTOR_MOVE_FORWARD;

  Serial.begin(115200);

  lastRun = millis();
}

#define COMMAND_ARGC(x) (x)

template<class T> T clamp(T value, T min, T max) {
  if( value < min ) {
    return min;
  }
  else if( value > max ) {
    return max;
  }
  return value;
}

void writeBluetoothCommand(const char *data ) {
  if( !data ) return;
  for( size_t i=0; data[i] != '\0'; i++ ) {
    bt.write(data[i]);
  }
  bt.write('\n');
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

/**
 * Read Bluetooth command
 */
bool readBluetoothCommand() {
  if( !bt.available() ) {
    return false;
  }

  const size_t BUFFSIZE = 64;
  const size_t COMMANDSIZE = 4;
  char buffer[BUFFSIZE];
  
  int i=0;
  int c=0;
  memset(buffer, 0, sizeof(char)*BUFFSIZE);
  char *commands[COMMANDSIZE];
  commands[0] = &buffer[0];
  for( i=0; bt.available() && i<(BUFFSIZE-1) && c<COMMANDSIZE; i++ ) {
    char in = bt.read();
    //bt.write(in);
    if( in == ':' ) {
      //bt.write(':');
      buffer[i] = '\0';
      commands[++c] = &buffer[i+1];
      continue;
    }
    else if( in == '\n' ) {
      // stop here
      buffer[i] = '\0';
      break;
    }
    else {
      //bt.write(i);
      buffer[i] = in;
    }

    // wait max 30ms for transmission
    for( int w=0; w<3 && !bt.available(); w++ ) {
      delay(10); 
    }
  }

  writeBluetoothCommand(commands[0]);
  writeBluetoothCommand(commands[1]);

  /*Serial.println("--- Commands received: ");
  Serial.println(commands[0]);
  Serial.println(commands[1]);
  Serial.println("-- END");*/
  
  if( strcmp(commands[0], "run") == 0 ) {
    /**
     * run:[0|1]
     */
    if( COMMAND_ARGC(c) != 1 ) {
      writeBluetoothCommand("run:error");
      return false;
    }
    
    short status = atoi(commands[1]);
    toggleRun(status == 0 ? false : true);
    
    writeBluetoothCommand("run:ok");
    //bt.write(status);
  }
  else if( strcmp(commands[0], "speed") == 0 ) {
    /**
     * speed:[0-255]
     */
    if( COMMAND_ARGC(c) != 1 ) {
      writeBluetoothCommand("speed:error");
      return false;
    }
    
    int value = atoi(commands[1]);
    speed = clamp(value, 0, 255);

    writeBluetoothCommand("speed:ok");
    //bt.write(speed);
  }
  else {
    writeBluetoothCommand(commands[0]);
  }

  return true;
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
  if( run && ((lastRun + 100) < now) ) {
    lastRun = now;

    int distance = ultrasonic_3.distanceCm();
    int go = direction;
    
    if( go == MOTOR_MOVE_FORWARD && distance < 30 ) {
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
    
    motor.move(go, speed);
    rgbled_7.show();
  }
  
}

