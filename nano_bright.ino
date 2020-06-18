/*  VERSION 2.0
 *
 *  Управление освещением яхты с HMI панели Nextion
 * 
 *  Диодная лента подключена через MOSFET транзисторы P0903BD
 *  на ШИМ выходы контроллера (Arduino Nano)
 *
 *  SERIAL - HMI NEXTION
 *  D6  - CABIN LIGHT LEFT SIDE
 *  D9  - CABIN LIGHT RIGHT SIDE
 *  D3  - BED LIGHT
 *  D10 - FLOOR LIGHT
 *  D5  - NAVIGATION LIGHTS
 * 
 */

#include <EEPROM.h>
#include <SoftwareSerial.h>

#define btnCABIN    1
#define btnFLOOR    2
#define btnBED      3
#define btnNAV      4
#define btnMAST     5

#define idCABIN     0x03  // 65 01 03
#define idCABIN_P   0x09
#define idCABIN_M   0x08

#define idFLOOR     0x07  // 65 01 07
#define idBED       0x04  // 65 01 04
#define idBED_P     0x0b
#define idBED_M     0x0a

#define idNAV       0x05  // 65 01 05
#define idMAST      0x06  // 65 01 06

#define UNDEFINE   0  //не определен
#define ON         1  //включение
#define OFF        2  //выключение

//SoftwareSerial SSerial(10, 11); // RX, TX

char SerialIn[64]; //буфер приема по serial портам
byte SerialInLen; //заполнение буфера
long SerialMillisRcv; //прием по 485 порту (отсрочка на прием всего пакета)

bool bt[100];

int dim[5]; //текущая яркость
int dimMode[5]; //выключение/выключение
int fade_amount=1; //шаг яркости

unsigned long ms_dim;
unsigned long ms_blink;

int touch_j=0; //счетчик для включения touch_j калибровки экрана


void setup() {
  Serial.begin(9600);
//  SSerial.begin(9600);
    
  pinMode(6, OUTPUT); //шим  было 2
  pinMode(9, OUTPUT); //шим  было 3
  pinMode(3, OUTPUT); //шим  было 4
  pinMode(10, OUTPUT); //шим  было 5
  pinMode(5, OUTPUT); //шим  было 6
  pinMode(13, OUTPUT); //led
  ms_blink = millis();
  ms_dim = millis();
  
  
  digitalWrite(10,HIGH);
  touch_j = EEPROM.read(0);
  if (touch_j>2) HmiCmd("touch_j"); //вызываем калибровку экрана
  touch_j++;
  EEPROM.write(0, touch_j);
  delay(3000);
  EEPROM.write(0, 0); //если питание не снято, то очищаем счетчик
  dimMode[0]=ON;
}


void loop() {

if (millis() - ms_dim > 20) {
  for (int i=0; i<5; i++){
    if (dimMode[i]==ON){
      dim[i]++;
      if (dim[i]>100) {
        dim[i] = 100;
        dimMode[i] = UNDEFINE;
      }
    }//ON
    if (dimMode[i]==OFF){
      dim[i]--;
      if (dim[i]<0) {
        dim[i] = 0;
        dimMode[i] = UNDEFINE;
      }
    }//OFF

//на nano как ШИМ работают выходы D3, D5, D6, D9, D10, D11
    analogWrite(6,dim[0]); //CABIN LEFT
    analogWrite(9,dim[0]); //CABIN RIGHT
    
    analogWrite(3,dim[1]); //BED
    analogWrite(10,dim[2]); //FLOOR
    analogWrite(5,dim[3]); //NAV

  } //for
  ms_dim = millis();
} //millis


// led blinking
  if (millis() - ms_blink > 3000){
    digitalWrite(13, HIGH);
  }
  if (millis() - ms_blink > 3100){
    digitalWrite(13, LOW);
    ms_blink = millis();
  }
  
//serial recieve
  while (Serial.available()) {
    char SerialChar = (char)Serial.read();
//    digitalWrite(13,HIGH);
//    SSerial.write(SerialChar);
//    digitalWrite(13,LOW);
    SerialIn[SerialInLen] = SerialChar;
    SerialInLen++;
    SerialMillisRcv = millis(); //для отсрочки обработки  
  }

  if (SerialInLen > 0 && (millis() - SerialMillisRcv > 200)) {
    
    if (SerialIn[2]==idCABIN && SerialIn[1]==0x01) { //CABIN
      bt[btnCABIN]=!bt[btnCABIN];
      if (bt[btnCABIN])  dimMode[0]=ON;
      if (!bt[btnCABIN]) dimMode[0]=OFF;
    }
    
    if (SerialIn[2]==idBED && SerialIn[1]==0x01) { //BED
      bt[btnBED]=!bt[btnBED];
      if (bt[btnBED])  dimMode[1]=ON;
      if (!bt[btnBED]) dimMode[1]=OFF;
    }
    
    if (SerialIn[2]==idFLOOR && SerialIn[1]==0x01) { //FLOOR
      bt[btnFLOOR]=!bt[btnFLOOR];
      if (bt[btnFLOOR])  dimMode[2]=ON;
      if (!bt[btnFLOOR]) dimMode[2]=OFF;
    }

    if (SerialIn[2]==idNAV && SerialIn[1]==0x01) { //NAV
      bt[btnNAV]=!bt[btnNAV];
      if (bt[btnNAV])  dimMode[3]=ON;
      if (!bt[btnNAV]) dimMode[3]=OFF;
    }

    if (SerialIn[2]==idMAST && SerialIn[1]==0x01) { //MAST
      bt[btnMAST]=!bt[btnMAST];
      if (bt[btnMAST])  dimMode[btnMAST]=ON;
      if (!bt[btnMAST]) dimMode[4]=OFF;
    }

    //ADJ + / -
    if (SerialIn[2]==idCABIN_M && SerialIn[1]==0x01) { //CABIN ADJ
      dim[0]=dim[0]-20;
      if (dim[0]<20) dim[0]=5;
      bt[btnCABIN]=1;
    }
    if (SerialIn[2]==idCABIN_P && SerialIn[1]==0x01) {
      dim[0]=dim[0]+20;
      if (dim[0]>100) dim[0]=100;
      bt[btnCABIN]=1;
    }

    if (SerialIn[2]==idBED_M && SerialIn[1]==0x01) { //BED ADJ
      dim[1]=dim[1]-20;
      if (dim[1]<20) dim[1]=5;
      bt[btnBED]=1;
    }
    if (SerialIn[2]==idBED_P && SerialIn[1]==0x01) {
      dim[1]=dim[1]+20;
      if (dim[1]>100) dim[1]=100;
      bt[btnBED]=1;
    }

    delay(10);
    if (bt[btnCABIN]) { HmiCmd("b1.val=1"); } else { HmiCmd("b1.val=0"); }
    if (bt[btnFLOOR]) { HmiCmd("b2.val=1"); } else { HmiCmd("b2.val=0"); }
    if (bt[btnBED]) { HmiCmd("b3.val=1"); } else { HmiCmd("b3.val=0"); }
    if (bt[btnNAV]) { HmiCmd("b4.val=1"); } else { HmiCmd("b4.val=0"); }
    if (bt[btnMAST]) { HmiCmd("b5.val=1"); } else { HmiCmd("b5.val=0"); }
    SerialInLen = 0;
  }

// --Перенаправление с SSerial на Serial
/*
  if (SSerial.available()) {
    digitalWrite(13,HIGH);
    Serial.write(SSerial.read());
    digitalWrite(13,LOW);
  }
*/

}

void HmiCmd(String Object)
{
  Serial.print(Object);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

void HmiPut(String Object,String Text)
{
  Serial.print(Object);
  Serial.write(0x22);
  Serial.print(Text);
  Serial.write(0x22);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}
