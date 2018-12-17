/*  VERSION 1.2
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



#define UNDEFINE   0  //не определен
#define ON         1  //включение
#define OFF        2  //выключение

char SerialIn[64]; //буфер приема по serial портам
byte SerialInLen; //заполнение буфера
long SerialMillisRcv; //прием по 485 порту (отсрочка на прием всего пакета)

bool bt[100];

int dim[5]; //текущая яркость
int dimMode[5]; //выключение/выключение
int fade_amount=1; //шаг яркости

unsigned long ms_dim;
unsigned long ms_blink;


void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT); //шим
  pinMode(3, OUTPUT); //шим
  pinMode(4, OUTPUT); //шим
  pinMode(5, OUTPUT); //шим
  pinMode(6, OUTPUT); //шим
  pinMode(13, OUTPUT); //led
  ms_blink = millis();
  ms_dim = millis();
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
    SerialIn[SerialInLen] = SerialChar;
    SerialInLen++;
    SerialMillisRcv = millis(); //для отсрочки обработки  
  }

  if (SerialInLen > 0 && (millis() - SerialMillisRcv > 200)) {
    
    if (SerialIn[2]==0x03) { //CABIN
      bt[3]=!bt[3];
      if (bt[3])  dimMode[0]=ON;
      if (!bt[3]) dimMode[0]=OFF;
    }
    
    if (SerialIn[2]==0x04) { //BED
      bt[4]=!bt[4];
      if (bt[4])  dimMode[1]=ON;
      if (!bt[4]) dimMode[1]=OFF;
    }
    
    if (SerialIn[2]==0x07) { //FLOOR
      bt[7]=!bt[7];
      if (bt[7])  dimMode[2]=ON;
      if (!bt[7]) dimMode[2]=OFF;
    }

    if (SerialIn[2]==0x05) { //NAV
      bt[5]=!bt[5];
      if (bt[5])  dimMode[3]=ON;
      if (!bt[5]) dimMode[3]=OFF;
    }

    if (SerialIn[2]==0x06) { //MAST
      bt[6]=!bt[6];
      if (bt[6])  dimMode[4]=ON;
      if (!bt[6]) dimMode[4]=OFF;
    }

    //ADJ + / -
    if (SerialIn[2]==0x08) { //CABIN ADJ
      dim[0]=dim[0]-20;
      if (dim[0]<20) dim[0]=5;
      bt[3]=1;
    }
    if (SerialIn[2]==0x09) {
      dim[0]=dim[0]+20;
      if (dim[0]>100) dim[0]=100;
      bt[3]=1;
    }

    if (SerialIn[2]==0x0A) { //BED ADJ
      dim[1]=dim[1]-20;
      if (dim[1]<20) dim[1]=5;
      bt[4]=1;
    }
    if (SerialIn[2]==0x0B) {
      dim[1]=dim[1]+20;
      if (dim[1]>100) dim[1]=100;
      bt[4]=1;
    }

   
    if (bt[3]) HmiCmd("bt3.val=1");
    if (bt[4]) HmiCmd("bt4.val=1");
    if (bt[5]) HmiCmd("bt5.val=1");
    if (bt[6]) HmiCmd("bt6.val=1");
    if (bt[7]) HmiCmd("bt7.val=1");
    SerialInLen = 0;
  }

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


