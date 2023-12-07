#include <SoftwareSerial.h>                                                 //Подключаем библиотеку

SoftwareSerial CMD(2, -1);  //RX, TX  PB1, PB0                              //Устанавливаем пины

void setup() { 
  CMD.begin(9600);
  pinMode(9, OUTPUT);                                                       //Объявляем пины и устанавливаем для них значение - низкий уровень сигнала
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(20, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);

  analogWrite(9, LOW);
  digitalWrite(8, LOW);
  digitalWrite(7, LOW);
  digitalWrite(20, LOW);
  digitalWrite(21, LOW);
  digitalWrite(22, LOW);
}

String getValue(String data, char separator, int index)                     //Получаем значение из строки по разделителю и позиции
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
 
String DATA = "";
void loop() {
  if (CMD.available()){
    DATA = CMD.readString();
    analogWrite(9, getValue(DATA,'x',0).toInt());                           //Включаем/выключаем соответствующие пины
    digitalWrite(8, getValue(DATA,'x',1).toInt()) ;
    digitalWrite(7, getValue(DATA,'x',2).toInt());
    digitalWrite(20, getValue(DATA,'x',3).toInt());
    digitalWrite(21, getValue(DATA,'x',4).toInt());
    digitalWrite(22, getValue(DATA,'x',5).toInt());
  }
}
