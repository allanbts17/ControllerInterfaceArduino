#include "ListLib.h"
#include <EEPROM.h>

const int MOTOR_MAYOR = 2;      //A mayor
const int MOTOR_MEDIANA = 3;    //B mediana
const int MOTOR_MENOR = 4;      //C menor
const int MARTILLO_MAYOR = 5;
const int MARTILLO_MEDIANA = 6;
const int MARTILLO_MENOR = 7;
const int RELOJ_A = 8;
const int RELOJ_B = 9;
const int BACKLIGHT = 10; //maybe the raspberry could do this
bool finished = false;
char exe[7];
int index=0;
int addr = 0;

unsigned long timeA = 0;
unsigned long timeB = 0;
unsigned long timeC = 0;
unsigned long noteTime = 1000;
unsigned long noteMillis = 0;
unsigned long timeForNextNote = 0;
bool nextNote = true;
bool activeA = false;
bool activeB = false;
bool activeC = false;
bool progressA = false;
bool progressB = false;
bool progressC = false;
bool start = false;

List<char> executionNotes;
List<int> executionTime;
List<int> executionDuration;

void serialFlush();
int freeRam ();
void pulseA(unsigned long noteDuration);
void pulseB(unsigned long noteDuration);
void pulseC(unsigned long noteDuration);
void executeToques();
void executeBandeo();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(MOTOR_MAYOR, OUTPUT);
  pinMode(MOTOR_MEDIANA, OUTPUT);
  pinMode(MOTOR_MENOR, OUTPUT);
  pinMode(MARTILLO_MAYOR, OUTPUT);
  pinMode(MARTILLO_MEDIANA, OUTPUT);
  pinMode(MARTILLO_MENOR, OUTPUT);
  pinMode(RELOJ_A, OUTPUT);
  pinMode(RELOJ_B, OUTPUT);

  for(int i=2;i<10;i++)
    digitalWrite(i,HIGH);
}

void loop() {

  if (Serial.available() > 6) {
    char *incomingBytes = new char[6];
    char *note = new char;
    note[0] = Serial.read();
    //Serial.readBytes(note,1);
    Serial.readBytes(incomingBytes,6);

    if(incomingBytes[0]=='n') {
      finished = true;
      executionNotes.Trim();
      executionTime.Trim();
      index = 0;
      Serial.print("Free ram: ");
      Serial.println(freeRam ());
    } 
    else {
      executionNotes.Add(note[0]);
      executionTime.Add(atoi(incomingBytes));
    }
  }

  if(finished){
    executeToques();
  }
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void pulseA(unsigned long noteDuration){
  if(activeA){
    if(millis() - noteMillis > timeForNextNote){
      digitalWrite(MARTILLO_MAYOR, LOW);
      timeA = millis();
      activeA = false;
      progressA = true;
      nextNote = true;
      index++;
    }
  }
  if(progressA)
    if(millis() - timeA >= noteDuration){
      digitalWrite(MARTILLO_MAYOR, HIGH);
      progressA = false;
    }
}

void pulseB(unsigned long noteDuration){
  if(activeB){
    if(millis() - noteMillis > timeForNextNote){
      digitalWrite(MARTILLO_MEDIANA, LOW);
      timeB = millis();
      activeB = false;
      progressB = true;
      nextNote = true;
      index++;
    }
  }
  if(progressB)
    if(millis() - timeB >= noteDuration){
      digitalWrite(MARTILLO_MEDIANA, HIGH);
      progressB = false;
    }
}

void pulseC(unsigned long noteDuration){
  if(activeC){    
    if(millis() - noteMillis > timeForNextNote){
      digitalWrite(MARTILLO_MENOR, LOW);
      timeC = millis();
      activeC = false;
      progressC = true;
      nextNote = true;
      index++;
      //avisar cuando index supera el limite
    }
  }
  if(progressC)
    if(millis() - timeC >= noteDuration){
      digitalWrite(MARTILLO_MENOR, HIGH);
      progressC = false;
    }
}

void executeToques(){
  unsigned long duration = 1000;

  //Update for next note
  if(nextNote && index < executionNotes.Count()){
    char note = executionNotes[index];
    timeForNextNote = executionTime[index];
    switch(note){
      case 'A':
        activeA = true;
        break;
      case 'B':
        activeB = true;
        break;
      case 'C':
        activeC = true;
        break;
    }
    noteMillis = millis();
    nextNote = false;
  }

  pulseA(duration);
  pulseB(duration);
  pulseC(duration);

  //Once last note is played.  Clear lists.
  if(index >= executionNotes.Count() && !progressA && !progressB && !progressC){
    finished = false;
    //posible liberación de memoria
    executionNotes.Clear();
    executionTime.Clear();
    nextNote = true;
    serialFlush();
  }
}
void executeBandeo(){}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}