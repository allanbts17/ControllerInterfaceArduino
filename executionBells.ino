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
const int RAM_MEMORY_LIMIT = 700;
const char ALLOW_COMMUNICATION = 'z';
int index=0;
char firstIncomingByte = ALLOW_COMMUNICATION;

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
bool finished = false;

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
void(* resetFunc) (void) = 0;

void setup() {
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

  if(Serial.available()>0 && firstIncomingByte == ALLOW_COMMUNICATION){
    firstIncomingByte = Serial.read();
  }
  if(firstIncomingByte == 'b'){
    if (Serial.available() > 13 && !finished) {
    char *incomingBytesForTime = new char[6];
    char *incomingBytesForDuration = new char[6];
    char *note = new char;
    note[0] = Serial.read();
    Serial.readBytes(incomingBytesForTime,6);
    Serial.readBytes(incomingBytesForDuration,6);

    if(incomingBytesForTime[0]=='n') {
      finished = true;
      executionNotes.Trim();
      executionTime.Trim();
      index = 0;
      Serial.print("Finished Free ram: ");
      Serial.println(freeRam ());
    } 
    else {
      executionNotes.Add(note[0]);
      executionTime.Add(atoi(incomingBytesForTime));
      executionDuration.Add(atoi(incomingBytesForDuration));
      delete note;
      delete [] incomingBytesForTime;
      delete [] incomingBytesForDuration;    
    }
  }
  } 
  else if (firstIncomingByte == 't'){
    if (Serial.available() > 6 && !finished) {
    char *incomingBytes = new char[6];
    char *note = new char;
    note[0] = Serial.read();
    Serial.readBytes(incomingBytes,6);

    if(incomingBytes[0]=='n') {
      finished = true;
      executionNotes.Trim();
      executionTime.Trim();
      index = 0;
      Serial.print("Finished Free ram: ");
      Serial.println(freeRam ());
    } 
    else {
      executionNotes.Add(note[0]);
      executionTime.Add(atoi(incomingBytes));
      delete note;
      delete [] incomingBytes;     
    }
  }
  }

  if(finished){
    if(firstIncomingByte == 't') executeToques();
    else executeBandeo();
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
    executionNotes.Clear();
    executionTime.Clear();
    //executionNotes.Trim(1);
    //executionTime.Trim(1);
    //executionNotes.freeMemory();
    //executionTime.freeMemory();
    nextNote = true;
    firstIncomingByte = ALLOW_COMMUNICATION;
    serialFlush();
    if(freeRam() < RAM_MEMORY_LIMIT)
      resetFunc();
  }
}
void executeBandeo(){
  static unsigned long fixedDurationA = 0;
  static unsigned long fixedDurationB = 0;
  static unsigned long fixedDurationC = 0;

  //Update for next note
  if(nextNote && index < executionNotes.Count()){
    char note = executionNotes[index];
    timeForNextNote = executionTime[index];
    
    switch(note){
      case 'A':
        activeA = true;
        fixedDurationA = executionDuration[index];
        break;
      case 'B':
        activeB = true;
        fixedDurationB = executionDuration[index];
        break;
      case 'C':
        activeC = true;
        fixedDurationC = executionDuration[index];
        break;
    }
    noteMillis = millis();
    nextNote = false;
  }

  pulseA(fixedDurationA);
  pulseB(fixedDurationB);
  pulseC(fixedDurationC);

  //Once last note is played.  Clear lists.
  if(index >= executionNotes.Count() && !progressA && !progressB && !progressC){
    finished = false;
    executionNotes.Clear();
    executionTime.Clear();
    executionDuration.Clear();
    //executionNotes.Trim(1);
    //executionTime.Trim(1);
    //executionNotes.freeMemory();
    //executionTime.freeMemory();
    nextNote = true;
    firstIncomingByte = ALLOW_COMMUNICATION;
    serialFlush();
    if(freeRam() < RAM_MEMORY_LIMIT)
      resetFunc();
  }
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}
