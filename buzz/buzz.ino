#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include <MeMCore.h>

MeBuzzer buzzer;

int numTones = 10;
int tones[] = {261, 277, 294, 311, 330, 349, 370, 392, 415, 440};
//            mid C  C#   D    D#   E    F    F#   G    G#   A
 
void setup()
{
  for (int i = 0; i < numTones; i++)
  {
    buzzer.tone(tones[i], 50);
  }
  buzzer.noTone();
}

void loop() {
  // put your main code here, to run repeatedly:

}
