
#include "ListLib.h"
#include <EEPROM.h>

const int Q1 = A0;
const int Q2 = A1;
const int Q3 = A2;
const int Q4 = A3;
const int STD = A4;
const int MOTOR_MAYOR = 2;      //A mayor
const int MOTOR_MEDIANA = 3;    //B mediana
const int MOTOR_MENOR = 4;      //C menor
const int MARTILLO_MAYOR = 5;
const int MARTILLO_MEDIANA = 6;
const int MARTILLO_MENOR = 7;
const int RELOJ_A = 8;
const int RELOJ_B = 9;
const int BACKLIGHT = 9; //its 10 //maybe the raspberry could do this
const int RAM_MEMORY_LIMIT = 700;
const char ALLOW_COMMUNICATION = 'z';
const char STOP_EXECUTION = 's';
const char EXECUTION_FINISHED = 'f';
const char RB_BYTE = 'B';
const char RA_BYTE = 'A';
const char BACKLIGHT_BYTE_ON = 'L';
const char BACKLIGHT_BYTE_OFF = 'l';
const char ARE_U_OK = '?';
int index=0;
char firstIncomingByte = ALLOW_COMMUNICATION;

unsigned long timeA = 0;
unsigned long timeB = 0;
unsigned long timeC = 0;
unsigned long noteTime = 1000;
unsigned long noteMillis = 0;
unsigned long timeForNextNote = 0;
unsigned long initMillisB;
unsigned long initMillisA;
bool nextNote = true;
bool activeA = false;
bool activeB = false;
bool activeC = false;
bool progressA = false;
bool progressB = false;
bool progressC = false;
bool start = false;
bool finished = false;
bool clockAOn = false;
bool clockAWait = false;
bool clockBOn = false;
bool clockBWait = false;
bool ifToques;

List<char> executionNotes;
List<int> executionTime;
List<int> executionDuration;

void serialFlush();
int freeRam ();
void pulseA(unsigned long noteDuration, int mayor);
void pulseB(unsigned long noteDuration, int mediana);
void pulseC(unsigned long noteDuration, int menor);
void executeToques();
void executeBandeo();
void phoneConnection();
void pause(unsigned long timeInMilliSeconds);
void(* resetFunc) (void) = 0;
void clockPulseA(unsigned long waitTime);
void clockPulseB(unsigned long waitTime);

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
  pinMode(Q1,INPUT);
  pinMode(Q2,INPUT);
  pinMode(Q3,INPUT);
  pinMode(Q4,INPUT);
  pinMode(STD,INPUT);

  for(int i=2;i<10;i++)
    digitalWrite(i,HIGH);
}


void loop() {
  phoneConnection();
  
  //Review the first incoming byte which tells the operation
  if(Serial.available()>0 && firstIncomingByte == ALLOW_COMMUNICATION){
    firstIncomingByte = Serial.read();
  } 
  
  //For regular clock pulse
  if(firstIncomingByte == RA_BYTE){
    clockAOn = true;
    firstIncomingByte = ALLOW_COMMUNICATION;
  }

  //For clock backlight
  else if (firstIncomingByte == BACKLIGHT_BYTE_ON){
    digitalWrite(BACKLIGHT,LOW);
    firstIncomingByte = ALLOW_COMMUNICATION;
  }
  else if (firstIncomingByte == BACKLIGHT_BYTE_OFF){
    digitalWrite(BACKLIGHT,HIGH);
    firstIncomingByte = ALLOW_COMMUNICATION;
  }

  else if(firstIncomingByte == ARE_U_OK){
    Serial.print(ARE_U_OK);
  }

  //For stop execution
  else if(firstIncomingByte == STOP_EXECUTION){
    finished = false;
    progressA = false;
    progressB = false;
    progressC = false;

    //Clearing lists
    executionNotes.Clear();
    executionTime.Clear();
    executionDuration.Clear();
    nextNote = true;
    serialFlush();
    firstIncomingByte = ALLOW_COMMUNICATION;

    digitalWrite(MOTOR_MAYOR,HIGH);
    digitalWrite(MOTOR_MEDIANA,HIGH);
    digitalWrite(MOTOR_MENOR,HIGH);
    digitalWrite(MARTILLO_MAYOR,HIGH);
    digitalWrite(MARTILLO_MEDIANA,HIGH);
    digitalWrite(MARTILLO_MENOR,HIGH);

    if(freeRam() < RAM_MEMORY_LIMIT)
      resetFunc();

    //Tell raspberry that execution has stopped
    Serial.print(EXECUTION_FINISHED);
  }

  //Imcoming bandeo
  else if(firstIncomingByte == 'b'){
    if (Serial.available() > 12 && !finished) {
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

      ifToques = false;
      firstIncomingByte = ALLOW_COMMUNICATION;
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

  //Incoming toque
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
      serialFlush();

      ifToques = true;
      firstIncomingByte = ALLOW_COMMUNICATION;
    } 
    else {
      executionNotes.Add(note[0]);
      executionTime.Add(atoi(incomingBytes));
      delete note;
      delete [] incomingBytes;     
    }
  }
  }

  //If a toque or bandeo sequence finished to arrive
  if(finished){
    if(ifToques) executeToques();
    else executeBandeo();
  }

  clockPulseA(1000);
  clockPulseB(1000);
}

void pause(unsigned long timeInMilliSeconds) {
	    unsigned long timestamp = millis();
	    do {
	    } while (millis() < timestamp + timeInMilliSeconds);
	}

void phoneConnection(){
  uint8_t number;
  bool signal;
  signal = digitalRead(STD);
  //signal = true;
  if(signal == HIGH){
    //pause(100);
    number = (0x00 | (digitalRead(Q1) << 0) | (digitalRead(Q2) << 1) | (digitalRead(Q3) << 2) | (digitalRead(Q4) << 3));
    //number = (0x00 | (0 << 0) | (0 << 1) | (1 << 2) | (0 << 3));
    switch (number)
    {
      case 0x01:
        Serial.print('1');
        break;
      case 0x02:
        Serial.print('2');
        break;
      case 0x03:
        Serial.print('3');
        break;
      case 0x04:
        Serial.print('4');
        //Serial.print(0x04);
        break;
      case 0x05:
        Serial.print('5');
        break;
      case 0x06:
        Serial.print('6');
        break;
      case 7:
        Serial.print('7');
        break;
      case 0x08:
        Serial.print('8');
        break;
      case 0x09:
        Serial.print('9');
        break;
      case 0x0A:
        Serial.print('0');
        break;
    }
  }
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void pulseA(unsigned long noteDuration, int mayor){
  if(activeA){
    if(millis() - noteMillis > timeForNextNote){
      digitalWrite(mayor, LOW);
      timeA = millis();
      activeA = false;
      progressA = true;
      nextNote = true;
      index++;
    }
  }
  if(progressA)
    if(millis() - timeA >= noteDuration){
      digitalWrite(mayor, HIGH);
      progressA = false;
    }
}

void pulseB(unsigned long noteDuration, int mediana){
  if(activeB){
    if(millis() - noteMillis > timeForNextNote){
      digitalWrite(mediana, LOW);
      timeB = millis();
      activeB = false;
      progressB = true;
      nextNote = true;
      index++;
    }
  }
  if(progressB)
    if(millis() - timeB >= noteDuration){
      digitalWrite(mediana, HIGH);
      progressB = false;
    }
}

void pulseC(unsigned long noteDuration, int menor){
  if(activeC){    
    if(millis() - noteMillis > timeForNextNote){
      digitalWrite(menor, LOW);
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
      digitalWrite(menor, HIGH);
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

  pulseA(duration,MARTILLO_MAYOR);
  pulseB(duration,MARTILLO_MEDIANA);
  pulseC(duration,MARTILLO_MENOR);

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
    serialFlush();
    
    //Tell raspberry
    Serial.print(EXECUTION_FINISHED);

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

  pulseA(fixedDurationA,MOTOR_MAYOR);
  pulseB(fixedDurationB,MOTOR_MEDIANA);
  pulseC(fixedDurationC,MOTOR_MENOR);

  //Once last note is played.  Clear lists.
  if(index >= executionNotes.Count() && !progressA && !progressB && !progressC){
    finished = false;
    executionNotes.Clear();
    executionTime.Clear();
    executionDuration.Clear();
    nextNote = true;

    //Tell raspberry
    Serial.print(EXECUTION_FINISHED);

    serialFlush();
    if(freeRam() < RAM_MEMORY_LIMIT)
      resetFunc();
  }
}

void serialFlush(){
  //Serial.println("erased");
  while(Serial.available() > 0) {
    char t = Serial.read();
    //Serial.println("erased");
  }
}

void clockPulseA(unsigned long waitTime){
  if(clockAOn){
    initMillisA = millis();
    digitalWrite(RELOJ_A,LOW);
    clockAWait = true;
    clockAOn = false;
  }

  if(clockAWait && millis() - initMillisA > waitTime){
    digitalWrite(RELOJ_A,HIGH);
    clockAWait = false;
    //serialFlush();
    firstIncomingByte = ALLOW_COMMUNICATION;
  }
}

void clockPulseB(unsigned long waitTime){
  if(clockBOn){
    initMillisB = millis();
    digitalWrite(RELOJ_B,LOW);
    clockBWait = true;
    clockBOn = false;
  }

  if(clockBWait && millis() - initMillisB > waitTime){
    digitalWrite(RELOJ_B,HIGH);
    clockBWait = false;
    //serialFlush();
    firstIncomingByte = ALLOW_COMMUNICATION;
  }

}
