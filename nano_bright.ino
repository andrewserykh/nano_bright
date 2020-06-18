/*  VERSION 2.2 (18.06.2020)
 *
 *  Управление освещением яхты с HMI панели Nextion
 * 
 *  Диодная лента подключена через MOSFET транзисторы P0903BD
 *  на ШИМ выходы контроллера (Arduino Nano)
 *  SSerial служит для трансляции команд с контроллера автопилота
 *  на HMI панель
 * 
 */

#include <EEPROM.h>
#include <SoftwareSerial.h>

//назначение выходов NANO на ШИМ
//на nano как ШИМ работают выходы D3, D5, D6, D9, D10, D11
#define pwmCABIN_LEFT   6
#define pwmCABIN_RIGHT  9
#define pwmBED          3
#define pwmFLOOR        10
#define pwmNAV          5

#define LED             13

//id элементов на HMI, для того чтобы "ловить" их в посылке при нажатии на панели
#define idCABIN         0x03  // 65 01 03
#define idCABIN_P       0x09  //кнопка ADJ+
#define idCABIN_M       0x08  //кнопка ADJ-
#define idFLOOR         0x07  // 65 01 07
#define idBED           0x04  // 65 01 04
#define idBED_P         0x0b  //ADJ+
#define idBED_M         0x0a  //ADJ-
#define idNAV           0x05  // 65 01 05
#define idMAST          0x06  // 65 01 06

#define btnCABIN        1 //номер кнопки на HMI панели
#define btnFLOOR        2 //прописывается для того, чтобы
#define btnBED          3 //при переключении страниц HMI-
#define btnNAV          4 //панели подсвечивались включенные
#define btnMAST         5 //кнопки

#define UNDEFINE        0  //не определен
#define ON              1  //включение
#define OFF             2  //выключение

SoftwareSerial SSerial(11, 12); // RX, TX

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
  SSerial.begin(9600);
  pinMode(pwmCABIN_LEFT, OUTPUT);
  pinMode(pwmCABIN_RIGHT, OUTPUT);
  pinMode(pwmBED, OUTPUT);
  pinMode(pwmFLOOR, OUTPUT);
  pinMode(pwmNAV, OUTPUT);
  pinMode(LED, OUTPUT); //LED на плате
  ms_blink = millis();
  ms_dim = millis();

  //для запуска калибровки HMI надо в течении 3 секунд несколько раз включить, выключить плату
  digitalWrite(LED, HIGH);
  touch_j = EEPROM.read(0);
  if (touch_j>2) HmiCmd("touch_j"); //вызываем калибровку экрана
  touch_j++;
  EEPROM.write(0, touch_j);
  delay(3000);
  EEPROM.write(0, 0); //если питание не снято, то очищаем счетчик
  digitalWrite(LED,LOW);
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

    analogWrite(pwmCABIN_LEFT,dim[0]);
    analogWrite(pwmCABIN_RIGHT,dim[0]);
    analogWrite(pwmBED,dim[1]);
    analogWrite(pwmFLOOR,dim[2]);
    analogWrite(pwmNAV,dim[3]);

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
    digitalWrite(LED,HIGH);
    SSerial.write(SerialChar);
    digitalWrite(LED,LOW);
    SerialIn[SerialInLen] = SerialChar;
    SerialInLen++;
    SerialMillisRcv = millis(); //для отсрочки обработки  
  }

  if (SerialInLen > 0 && (millis() - SerialMillisRcv > 200)) {
    //_______________id элемента_____________id страницы____
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

    delay(10); //пауза для того чтобы HMI успел переключить страницу и объекты обновились
    if (bt[btnCABIN]) { HmiCmd("b1.val=1"); } else { HmiCmd("b1.val=0"); }
    if (bt[btnFLOOR]) { HmiCmd("b2.val=1"); } else { HmiCmd("b2.val=0"); }
    if (bt[btnBED]) { HmiCmd("b3.val=1"); } else { HmiCmd("b3.val=0"); }
    if (bt[btnNAV]) { HmiCmd("b4.val=1"); } else { HmiCmd("b4.val=0"); }
    if (bt[btnMAST]) { HmiCmd("b5.val=1"); } else { HmiCmd("b5.val=0"); }
    SerialInLen = 0;
  }

// --перенаправление с SSerial на Serial
  if (SSerial.available()) {
    digitalWrite(LED,HIGH);
    Serial.write(SSerial.read());
    digitalWrite(LED,LOW);
  }

}

//отправка произвольной команды на HMI
void HmiCmd(String Object)
{
  Serial.print(Object);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

//отправка строки в элемент (передает "")
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
