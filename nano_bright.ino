/*  VERSION 3.3 (13.07.2020) OLDBOOTLOADER
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

#define T_MS_DIM        10 //обновление текущей яркости (скорость включения/выключения)
//назначение выходов NANO на ШИМ
//на nano как ШИМ работают выходы D3, D5, D6, D9, D10, D11
#define pwmCABIN_LEFT   9  //CABIN LEFT?    6
#define pwmCABIN_RIGHT  3  //CABIN LEFT?    9
#define pwmBED          10  //CABIN RIGHT    3
#define pwmFLOOR        5  //BED            10
#define pwmNAV          6  //FLOOR          5

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

#define SLEEP           1 //для переменной SmartMode
#define CHILL           2
#define PULSE           3
#define WAVE            4

#define idSLEEP         0x03 //id с HMI панели 0x02 - экран
#define idCHILL         0x04
#define idPULSE         0x05
#define idWAVE          0x07
#define idEMPTY1        0x08
#define idEMPTY2        0x09
#define idEMPTY3        0x0a
#define idEMPTY4        0x0b

#define btnCABIN        1 //номер кнопки на HMI панели
#define btnFLOOR        2 //прописывается для того, чтобы
#define btnBED          3 //при переключении страниц HMI-
#define btnNAV          4 //панели подсвечивались включенные
#define btnMAST         5 //кнопки

#define btnSLEEP        6 
#define btnCHILL        7 
#define btnPULSE        8 
#define btnWAVE         9 

#define ON              100  //включение
#define OFF             0    //выключение

#define CABIN_LEFT      0  //в массиве dim
#define CABIN_RIGHT     1
#define BED             3
#define FLOOR           4
#define NAV             5
#define MAST            6

#define RX_BUFFER       256

SoftwareSerial SSerial(11, 12); // RX, TX

char SerialIn[RX_BUFFER]; //буфер приема по serial портам
byte SerialInLen; //заполнение буфера
long SerialMillisRcv; //прием по 485 порту (отсрочка на прием всего пакета)

char SSerialIn[RX_BUFFER]; //буфер приема по serial портам
byte SSerialInLen; //заполнение буфера
long SSerialMillisRcv; //прием по 485 порту (отсрочка на прием всего пакета)

bool bt[100];

int dim[10]; //текущая яркость
int dimSet[10]; //установка яркости
int fade_amount=1; //шаг яркости

unsigned long ms_dim;
unsigned long ms_blink;

int SmartMode = 0; //автоматические режимы подсветки
unsigned long ms_smart; //millis для обновления смарт-подсветки
unsigned long t_ms_smart; //интервал

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

if (millis() - ms_dim > T_MS_DIM) { //обновление текущей яркости
  for (int i=0; i<10; i++){ //перебор массива dim
    //"догоняем" установленное значение яркости
    if (dimSet[i] < OFF) dimSet[i] = OFF;
    if (dim[i] < dimSet[i]) dim[i] = dim[i] + fade_amount;
    if (dim[i] > dimSet[i]) dim[i] = dim[i] - fade_amount;
    if (dim[i] < OFF) dim[i] = OFF;
    if (dim[i] > ON) dim[i] = ON;
    //передаем на шим
    analogWrite(pwmCABIN_LEFT,dim[CABIN_LEFT]);
    analogWrite(pwmCABIN_RIGHT,dim[CABIN_RIGHT]);
    analogWrite(pwmBED,dim[BED]);
    analogWrite(pwmFLOOR,dim[FLOOR]);
    analogWrite(pwmNAV,dim[NAV]);
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
    SerialIn[SerialInLen] = SerialChar;
    if (SerialInLen < RX_BUFFER) SerialInLen++;
    SerialMillisRcv = millis(); //для отсрочки обработки  
  }

  if (SerialInLen > 0 && (millis() - SerialMillisRcv > 200)) {
    //_______________id элемента_____________id страницы____
    if (SerialIn[2]==idCABIN && SerialIn[1]==0x01) { //CABIN
      bt[btnCABIN]=!bt[btnCABIN];
      if (bt[btnCABIN])  dimSet[CABIN_LEFT]=ON;
      if (bt[btnCABIN])  dimSet[CABIN_RIGHT]=ON;
      if (!bt[btnCABIN]) dimSet[CABIN_LEFT]=OFF;
      if (!bt[btnCABIN]) dimSet[CABIN_RIGHT]=OFF;      
    }
    
    if (SerialIn[2]==idBED && SerialIn[1]==0x01) { //BED
      bt[btnBED]=!bt[btnBED];
      if (bt[btnBED])  dimSet[BED]=ON;
      if (!bt[btnBED]) dimSet[BED]=OFF;
    }
    
    if (SerialIn[2]==idFLOOR && SerialIn[1]==0x01) { //FLOOR
      bt[btnFLOOR]=!bt[btnFLOOR];
      if (bt[btnFLOOR])  dimSet[FLOOR]=ON;
      if (!bt[btnFLOOR]) dimSet[FLOOR]=OFF;
    }

    if (SerialIn[2]==idNAV && SerialIn[1]==0x01) { //NAV
      bt[btnNAV]=!bt[btnNAV];
      if (bt[btnNAV])  dimSet[NAV]=ON;
      if (!bt[btnNAV]) dimSet[NAV]=OFF;
    }

    if (SerialIn[2]==idMAST && SerialIn[1]==0x01) { //MAST
      bt[btnMAST]=!bt[btnMAST];
      if (bt[btnMAST])  dimSet[MAST]=ON;
      if (!bt[btnMAST]) dimSet[MAST]=OFF;
    }

    //ADJ + / -
    if (SerialIn[2]==idCABIN_M && SerialIn[1]==0x01) { //CABIN ADJ
      dimSet[CABIN_LEFT]=dimSet[CABIN_LEFT]-20;
      if (dimSet[CABIN_LEFT]<20) dimSet[CABIN_LEFT]=5;
      dimSet[CABIN_RIGHT]=dimSet[CABIN_RIGHT]-20;
      if (dimSet[CABIN_RIGHT]<20) dimSet[CABIN_RIGHT]=5;
      bt[btnCABIN]=1;
    }
    if (SerialIn[2]==idCABIN_P && SerialIn[1]==0x01) {
      dimSet[CABIN_LEFT]=dimSet[CABIN_LEFT]+20;
      if (dimSet[CABIN_LEFT]>ON) dimSet[CABIN_LEFT]=ON;
      dimSet[CABIN_RIGHT]=dimSet[CABIN_RIGHT]+20;
      if (dimSet[CABIN_RIGHT]>ON) dimSet[CABIN_RIGHT]=ON;
      bt[btnCABIN]=1;
    }

    if (SerialIn[2]==idBED_M && SerialIn[1]==0x01) { //BED ADJ
      dimSet[BED]=dimSet[BED]-20;
      if (dimSet[BED]<20) dimSet[BED]=5;
      bt[btnBED]=1;
    }
    if (SerialIn[2]==idBED_P && SerialIn[1]==0x01) {
      dimSet[BED]=dimSet[BED]+20;
      if (dimSet[BED]>ON) dimSet[BED]=ON;
      bt[btnBED]=1;
    }

   if (SerialIn[2]==idSLEEP && SerialIn[1]==0x02) { //SMART MENU ---
      bt[btnSLEEP]=!bt[btnSLEEP];
      if (bt[btnSLEEP]) {
        SmartMode=SLEEP;
        dimSet[CABIN_LEFT] = 50;
        dimSet[CABIN_RIGHT] = 50;
        dimSet[BED] = 100;
        dimSet[FLOOR] = 100;
        t_ms_smart = 30000; //интервал обновление 30 секунд
        ms_smart = millis();
      }
      if (!bt[btnSLEEP]) SmartMode=0;
    }

    if (SerialIn[2]==idCHILL && SerialIn[1]==0x02) { //SMART MENU ---
      bt[btnCHILL]=!bt[btnCHILL];
      if (bt[btnCHILL]) {
        SmartMode=CHILL;
        dimSet[CABIN_LEFT]=random(40,50);
        dimSet[CABIN_RIGHT]=random(40,50);
        dimSet[BED]=random(10,30);
        dimSet[FLOOR]=random(60,100);
        t_ms_smart = 200; //интервал обновления смарт подсветки
        ms_smart = millis();
      }
      if (!bt[btnCHILL]) SmartMode=0;
    }

    if (SerialIn[2]==idPULSE && SerialIn[1]==0x02) { //SMART MENU ---
      bt[btnPULSE]=!bt[btnPULSE];
      if (bt[btnPULSE]) {
        SmartMode=PULSE;
        dimSet[CABIN_LEFT] = 50;
        dimSet[CABIN_RIGHT] = 30;
        dimSet[BED] = 20;
        dimSet[FLOOR] = 100;        
        t_ms_smart = 500; //интервал обновления смарт подсветки
        ms_smart = millis();
      }
      if (!bt[btnPULSE]) SmartMode=0;
    }

    delay(20); //пауза для того чтобы HMI успел переключить страницу и объекты обновились
    if (bt[btnCABIN]) { HmiCmd("b1.val=1"); } else { HmiCmd("b1.val=0"); }
    if (bt[btnFLOOR]) { HmiCmd("b2.val=1"); } else { HmiCmd("b2.val=0"); }
    if (bt[btnBED]) { HmiCmd("b3.val=1"); } else { HmiCmd("b3.val=0"); }
    if (bt[btnNAV]) { HmiCmd("b4.val=1"); } else { HmiCmd("b4.val=0"); }
    if (bt[btnMAST]) { HmiCmd("b5.val=1"); } else { HmiCmd("b5.val=0"); }

    SSerial.write(SerialIn); //отправка принятого в SSerial
    SerialInLen = 0;
  }

// --перенаправление с SSerial на Serial
  while (SSerial.available()) {
    digitalWrite(LED,HIGH);
    char SSerialChar = (char)SSerial.read();
    SSerialIn[SSerialInLen] = SSerialChar;
    if (SSerialInLen < RX_BUFFER) SSerialInLen++;
    SSerialMillisRcv = millis(); //для отсрочки обработки
    digitalWrite(LED,LOW);

  }

  if (SSerialInLen > 0 && (millis() - SSerialMillisRcv > 200)) {
      Serial.print(SSerialIn);
      SSerialInLen = 0;
  }

  if (SmartMode == SLEEP && millis() - ms_smart > t_ms_smart){
    dimSet[CABIN_LEFT]=dimSet[CABIN_LEFT]-20;
    dimSet[CABIN_RIGHT]=dimSet[CABIN_RIGHT]-20;
    dimSet[BED]=dimSet[BED]-20;
    dimSet[FLOOR]=dimSet[FLOOR]-10;
    ms_smart = millis();  
  } //SmartMode

  if (SmartMode == PULSE && millis() - ms_smart > t_ms_smart){
    dimSet[CABIN_LEFT]=random(100);
    dimSet[CABIN_RIGHT]=random(100);
    dimSet[BED]=random(100);
    dimSet[FLOOR]=random(100);
    ms_smart = millis();
  } //SmartMode

  if (SmartMode == CHILL && millis() - ms_smart > t_ms_smart){
    dimSet[CABIN_LEFT]=random(40,50);
    dimSet[CABIN_RIGHT]=random(40,50);
    dimSet[BED]=random(10,30);
    dimSet[FLOOR]=random(60,100);
    ms_smart = millis();
  } //SmartMode

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

//отправка строки в элемент (передает "")
void HmiPutFloat(String Object,float Value)
{
  Serial.print(Object);
  Serial.write(0x22);
  Serial.print(Value);
  Serial.write(0x22);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}
