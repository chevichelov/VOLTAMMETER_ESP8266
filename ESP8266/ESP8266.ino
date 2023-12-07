 #include "Free_Fonts.h"
#include "Icons.h" 
#include "Save.h" 
#include "Button.h" 
#include "Time.h"
#include "Data.h"
#include "To.h"
#include "time.h"

#include <Ds1302.h>

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <uri/UriBraces.h>

#include <SPI.h>
#include <TFT_eSPI.h>

#include <INA226_WE.h>
#include <Wire.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <EEPROM.h>
#include <SD.h>

#define EEPROM_LIMIT 300
#define SET_CORRECTION_FACTOR 16.95                           //Калибровочный коэффициент силы тока
#define SET_TIMEZONE 18000                                    //Часовой пояс 60 сек. * 60 мин. * 5 часов = 18000



SAVE SAVE;                                                    //Объявляем класс сохранения данных

WiFiUDP NTP_UDP;
NTPClient TIME_CLIENT(NTP_UDP, "pool.ntp.org", SET_TIMEZONE);  

#define CLKPin 10                                             // Chip Enable
#define DATPin 5                                              // Input/Output
#define RSTPin -1                                             // Serial Clock
Ds1302 RTC(RSTPin, CLKPin, DATPin);

uint8_t arduino_mac[] = {SAVE.MAC[0], SAVE.MAC[1], SAVE.MAC[2], SAVE.MAC[3], SAVE.MAC[4], SAVE.MAC[5]};
IPAddress IP(SAVE.IP[0], SAVE.IP[1], SAVE.IP[2], SAVE.IP[3]);
IPAddress GATEWAY(SAVE.GATEWAY[0], SAVE.GATEWAY[1], SAVE.GATEWAY[2], SAVE.GATEWAY[3]);
IPAddress SUBNET(SAVE.SUBNET[0], SAVE.SUBNET[1], SAVE.SUBNET[2], SAVE.SUBNET[3]);
IPAddress DNS(SAVE.DNS[0], SAVE.DNS[1], SAVE.DNS[2], SAVE.DNS[3]);
ESP8266WebServer SERVER(SAVE.PORT);

uint16_t CAL_DATA[5];                                           //Данные калибровки тачпад
TFT_eSPI TFT = TFT_eSPI();

INA226_WE INA226 = INA226_WE();

OneWire oneWire(PIN_D1);                                        // Настраиваем экземпляр oneWire для связи с устройством OneWire
DallasTemperature SENSORS(&oneWire);                            // Передаем ссылку oneWire датчику температуры
DeviceAddress THERMOMETER;                                      //={ 0x28, 0x96, 0x3, 0x57, 0x4, 0xE1, 0x3C, 0x3F }

Ds1302::DateTime NOW;

TIME TIME;                                                      //Объявляем класс времени
DATA DATA;                                                      //Объявляем класс информации
TO TO;                                                          //Объявляем класс конвертирования данных

/*Объявляем класс кнопки*/
BUTTON MENU_BUTTON(290, 0, 30, 30);                             //Кнопка меню
BUTTON WIFI_BUTTON(245, 0, 30, 30);                             //Кнопка включения/выключения WIFI
BUTTON GRAPH_BUTTON(200, 0, 30, 30);                            //Кнопка включения/выключения графика

BUTTON QC_BUTTON(0, 210, 30, 30);                               //Кнопка включения/выключения быстрая зарядка QC 2.0 QC 3.0
BUTTON QI_BUTTON(40, 210, 30, 30);                              //Кнопка включения/выключения Бесправодная зарядки QI
BUTTON ST_BUTTON(80, 210, 30, 30);                              //Кнопка включения/выключения серво тестер
BUTTON MT_BUTTON(120, 210, 30, 30);                             //Кнопка включения/выключения мотор тестнр
BUTTON EL_BUTTON(160, 210, 30, 30);                             //Кнопка включения/выключения Резервная

BUTTON DELETE_BUTTON(160, 65, 160, 30);                         //Кнопка удаления
BUTTON NO_BUTTON(160, 65, 76, 30);                              //Кнопка отмены
BUTTON YES_BUTTON(240, 65, 76, 30);                             //Кнопка согласия

BUTTON OFF_ON_BUTTON(240, 114, 76, 30);                         //Кнопка включения/отключения

BUTTON VOLTAGE_BUTTON(120, 158, 30, 30);                        //Кнопка выбора цвета напряжения
BUTTON AMP_BUTTON(286, 158, 30, 30);                            //Кнопка выбора цвета силы тока
BUTTON WATTS_BUTTON(120, 208, 30, 30);                          //Кнопка выбора цвета ватт
BUTTON GRID_GRAPHICS_BUTTON(286, 208, 30, 30);                  //Кнопка выбора цвета решотки графика

#include "WebServer.h"


void setup(void) {
  Serial.begin(9600);
  TFT.begin();                                                  //Выводим данные на экран
  TFT.setRotation(3);                                           //Положение экрана определяется цифрами 1 или 3, но после переворачивания нужно сделать калибровку сенсора
  TFT.fillScreen(TFT_BLACK);

  const __FlashStringHelper* OK = F("OK");                      //Экономим оперативную память и сохраняем во флеш память
  const __FlashStringHelper* NO = F("NO");                      //Экономим оперативную память и сохраняем во флеш память

  DATA.HEIGHT = TFT.height();                                   //Рассчитываем высоту экрана
  DATA.HEIGHT_GRAPH = DATA.HEIGHT - 20;
  DATA.WIDGHT = TFT.width();                                    //Рассчитываем ширину экрана
  
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("youtube.com/@chevichelov"), TFT.width() / 4, 0, FONT2);
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("Display..............................................................."), 0, 15, FONT2);
  TFT.setTextColor(TFT_GREEN, TFT_BLACK);
  TFT.drawString(OK, TFT.width()-15, 15, FONT2);               //Экран инициализирован успешно


  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("EEPROM..............................................................."), 0, 30, FONT2);

  EEPROM.begin(1);
  EEPROM.get(0, DATA.IS_CLEAR_EEPROM);
  EEPROM.end();
  if (DATA.IS_CLEAR_EEPROM == 3)
  {
    EEPROM_GET();
    ++SAVE.IS_TOUCH;
  }
  else
  {
    DATA.IS_CLEAR_EEPROM = 3;
    EEPROM.begin(1);
    EEPROM.put(0, DATA.IS_CLEAR_EEPROM);
    EEPROM.commit();
    EEPROM.end();
  }
  EEPROM_SAVE();
  WIFI_BUTTON.IS_PRESSED = SAVE.IS_WIFI;
  Wire.begin(PIN_D1, PIN_D2);
  TFT.setTextColor(TFT_GREEN, TFT_BLACK);
  TFT.drawString(OK, TFT.width()-15, 30, FONT2);                  //EEPROM инициализирован успешно


  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("INA226..............................................................."), 0, 45, FONT2);
  if (INA226.init() == 1)
  {
    INA226.setConversionTime(CONV_TIME_8244);                     //Время конверсии в микросекундах (140,204,332,588,1100,2116,4156,8244)8244µs=8.244 ms
    INA226.setAverage(AVERAGE_4);                                 //Среднее количество чтений n раз (1,4,16,64,128,256,512,1024)
    INA226.setMeasureMode(CONTINUOUS);
    
    TFT.setTextColor(TFT_GREEN, TFT_BLACK);
    TFT.drawString(OK, TFT.width()-15, 45, FONT2);
  }
  else
  {
    TFT.setTextColor(TFT_RED, TFT_BLACK);
    TFT.drawString(NO, TFT.width()-15, 45, FONT2);
  }

  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("RTC..............................................................."), 0, 60, FONT2);
  RTC.init();
  TFT.setTextColor(TFT_GREEN, TFT_BLACK);
  TFT.drawString(OK, TFT.width()-15, 60, FONT2);                  //Модуль реального времени инициализирован успешно


  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("SENSORS..............................................................."), 0, 75, FONT2);
  SENSORS.begin();
  if (SENSORS.getDeviceCount() > 0)
  {
    SENSORS.getAddress(THERMOMETER, 0);
    TFT.setTextColor(TFT_GREEN, TFT_BLACK);
    TFT.drawString(OK, TFT.width()-15, 75, FONT2);                //Датчик температуры инициализирован успешно
  }
  else
  {
    TFT.setTextColor(TFT_RED, TFT_BLACK);
    TFT.drawString(NO, TFT.width()-15, 75, FONT2);                //Датчик температуры инициализирован не успешно
  }


  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("TOUCH..............................................................."), 0, 90, FONT2);
  if (SAVE.IS_TOUCH >= 5 || (sizeof(SAVE.CAL_DATA) / sizeof(uint16_t) == 5 && ((SAVE.CAL_DATA[0] > 10000 && SAVE.CAL_DATA[1] > 10000 && SAVE.CAL_DATA[2] > 10000 && SAVE.CAL_DATA[3] > 10000 && SAVE.CAL_DATA[4] > 10000) || (SAVE.CAL_DATA[0] == 0 || SAVE.CAL_DATA[1] == 0 || SAVE.CAL_DATA[2] == 0 || SAVE.CAL_DATA[3] == 0 || SAVE.CAL_DATA[4] == 0))))
    TOUCH_CALIBRATE();
  else
  {
    memcpy(CAL_DATA, SAVE.CAL_DATA, sizeof(SAVE.CAL_DATA));
    TFT.setTouch(CAL_DATA);
  }
  TFT.setTextColor(TFT_GREEN, TFT_BLACK);
  TFT.drawString(OK, TFT.width()-15, 90, FONT2);


  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("SD..............................................................."), 0, 105, FONT2);
  if(!SD.begin(3)){
    TFT.setTextColor(TFT_RED, TFT_BLACK);
    TFT.drawString(F("FATAL ERROR NO SD CARD........................................."), 0, 105, FONT2);
    while(true)                                                   //SD карта инициализирована не успешно
      delay(1);
  }
  else
  {
    fs::File ROOT = SD.open("/");
    if (!SD.exists("DATA"))
      SD.mkdir("DATA");
    DATA.SIZE = 0;
    SIZE_DIRECTORY(ROOT, 0); 
    if (DATA.SIZE > 34254880768) // 32ГБ - 100МБ
      SAVE.SD_WRITE = false;
    TFT.setTextColor(TFT_GREEN, TFT_BLACK);
    TFT.drawString(OK, TFT.width()-15, 105, FONT2);               //SD карта инициализирована успешно
  }

  
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  uint16_t WiFi_STATUS = 0;
  if (SAVE.IS_WIFI)                                              //Если Wi-Fi включен - инициализируем его и подключиться к Wi-Fi
  {
    TFT.drawString(F("Wi-Fi"), 0, 120, FONT2);
    
    WiFi.begin(SAVE.SSID, SAVE.PASSWORD);
    WiFi.config(IP, GATEWAY, SUBNET, DNS);
   
    while (WiFi.status() != WL_CONNECTED && WiFi_STATUS != SAVE.CONNECTED_COUNT) {
      delay(10);
      TFT.drawString(F("."), (WiFi_STATUS * 5)+ 30, 120, FONT2);
      ++WiFi_STATUS;
    }
    if (WiFi_STATUS == SAVE.CONNECTED_COUNT)
    {
      TFT.setTextColor(TFT_RED, TFT_BLACK);
      TFT.drawString(NO, TFT.width()-15, 120, FONT2);
    }
    else
    {
      TFT.setTextColor(TFT_GREEN, TFT_BLACK);
      TFT.drawString(OK, TFT.width()-15, 120, FONT2);
    }
  }
  else
  {
    TFT.drawString(F("WiFi...................................................."), 0, 120, FONT2);
    TFT.setTextColor(TFT_RED, TFT_BLACK);
    TFT.drawString(NO, TFT.width()-15, 120, FONT2);
    DATA.STRING_IP = "WI-FI OFF";                                   //Если Wi-Fi не включен, вывод об этом информацию
  }
  

  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("RTC CONNECTION...................................................."), 0, 135, FONT2);
  TFT.setTextColor(TFT_RED, TFT_BLACK);
  if (WiFi_STATUS != SAVE.CONNECTED_COUNT && SAVE.IS_WIFI)          //Если смогли подключиться к Wi-Fi, то синхронизируем время на модуле реальногов ремени с мировым временем
  {
    DATA.STRING_IP = WiFi.localIP().toString();
    TIME_CLIENT.begin();
    TIME_CLIENT.update();
    time_t epochTime = TIME_CLIENT.getEpochTime();
    struct tm *DATE = gmtime ((time_t *)&epochTime); 
    
    Ds1302::DateTime DT = {
      .year   = (DATE->tm_year + 1900) - 2000,
      .month  = DATE->tm_mon + 1,
      .day    = DATE->tm_mday,
      .hour   = TIME_CLIENT.getHours(),
      .minute = TIME_CLIENT.getMinutes(),
      .second = TIME_CLIENT.getSeconds(),
      .dow    = TIME_CLIENT.getDay()
    };
    RTC.setDateTime(&DT);
    RTC.getDateTime(&NOW);
    DATA.STRING_DATE = TO.DISPLAY_DATE(NOW.day) +"."+ TO.DISPLAY_DATE(NOW.month) +"."+ TO.DISPLAY_DATE(NOW.year) +" "+ TO.DISPLAY_DATE(NOW.hour) +":"+ TO.DISPLAY_DATE(NOW.minute) +":"+ TO.DISPLAY_DATE(NOW.second);
    Serial.println("START: " + DATA.STRING_DATE);                   //Выводим дату и время на экран
    TFT.setTextColor(TFT_GREEN, TFT_BLACK);
    TFT.drawString(OK, TFT.width()-15, 135, FONT2);
  }
  else
  {
    TFT.setTextColor(TFT_GREEN, TFT_BLACK);
    TFT.drawString(NO, TFT.width()-15, 135, FONT2);                //Выводим информацию о том, что не производили синхронизацию времени на модуле реального времени с серверами мирового времени
  }
 
  if (WiFi_STATUS == SAVE.CONNECTED_COUNT && SAVE.IS_WIFI)         //Если не смогли подключиться к WI-FI то создаём свою точку доступа youtube_com_chevichelov
  {
    WiFi.softAP("youtube_com_chevichelov", SAVE.PASSWORD);
    WiFi.softAPConfig(IP, GATEWAY, SUBNET);
    IPAddress IP = WiFi.softAPIP(); 
    DATA.STRING_IP = String() + IP[0] + "." + IP[1] + "." + IP[2] + "." + IP[3];
  }

  
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.drawString(F("SERVER..............................................................."), 0, 150, FONT2);

  if (SAVE.IS_WIFI)                                                 //Если создали свою точку доступа то нам доступна только страница настроек
  {
    if (WiFi_STATUS == SAVE.CONNECTED_COUNT)
    {
      SERVER.on("/", [](){
        SERVER.send(200, "text/html; charset=utf-8", GET_FILE("/settings_only.html"));
      });
    }
    else                                                            //Если подключились к точке доступа то нам доступны все страницы вольтамперметра
    {
      SERVER.on("/", [](){
        SERVER.send(200, "text/html; charset=utf-8", GET_FILE("/index.html"));                            //Главная страница index.html
      });
      SERVER.on("/js/chart.js", [](){
        SERVER.send(200, "application/javascript; charset=utf-8", chart_js);                              //Файл chart.js
      });
      SERVER.on("/js/hammer.min.js", [](){
        SERVER.send(200, "application/javascript; charset=utf-8", hammer_min_js);                         //Файл hammer_min.js
      });
      SERVER.on("/js/chartjs-plugin-zoom.min.js", [](){
        SERVER.send(200, "application/javascript; charset=utf-8", chartjs_plugin_zoom_min_js);            //Файл hammer_min.js
      });
      SERVER.on("/online", [](){
        SERVER.send(200, "application/json; charset=utf-8", "{\"DATE\":\"" + DATA.STRING_DATE + "\", \"VOLT\":" + DATA.VOLTS + ", \"VOLT_COLOR\":\"" + SAVE.VOLTAGE_COLOR[0] + "," + SAVE.VOLTAGE_COLOR[1] + "," + SAVE.VOLTAGE_COLOR[2] + "\", \"AMP\":" + DATA.AMPS + ", \"AMP_COLOR\":\"" + SAVE.AMP_COLOR[0] + "," + SAVE.AMP_COLOR[1] + "," + SAVE.AMP_COLOR[2] + "\", \"WATT\":" + DATA.WATTS + ", \"WATT_COLOR\":\"" + SAVE.WATTS_COLOR[0] + "," + SAVE.WATTS_COLOR[1] + "," + SAVE.WATTS_COLOR[2] + "\", \"TEMPERATURE\":" + DATA.TEMPERATURE + "}");
      });                                                                                                 //Данные вольтамперметра
      SERVER.on("/log", [](){
        SERVER.send(200, "application/json; charset=utf-8", LIST_DIRECTORY());                            //Какое пространство занято на SD карте
      });
      SERVER.on("/logs", [](){
        SERVER.send(200, "text/html; charset=utf-8", GET_FILE("/logs.html"));                             //Страница логов
      });
      SERVER.on(UriBraces("/file/{}"), [](){
        SERVER.send(200, "application/json; charset=utf-8", GET_FILE_CSV("/DATA/" + SERVER.pathArg(0)));  //Содержимое файла лога
      });
      SERVER.on(UriBraces("/files/{}"), [](){
        SERVER.send(200, "text/html; charset=utf-8", GET_FILE("/files.html"));                            //Страница файла лога
      });
      SERVER.on("/settings", [](){
        SERVER.send(200, "text/html; charset=utf-8", GET_FILE("/settings.html"));                         //Страница настроек
      });
    }
    SERVER.on("/favicon.ico", [](){
      SERVER.send(200, "image/x-icon", GET_FILE("favicon.ico"));                                          //Файл favicon.ico
    });
    SERVER.on("/loadsetting", [](){
      SERVER.send(200, "application/json; charset=utf-8", "{\"SSID\":\"" + String(SAVE.SSID) + "\", \"PASSWORD\":\"" + String(SAVE.PASSWORD) + "\", \"CONNECTED_COUNT\":\"" + SAVE.CONNECTED_COUNT + "\", \"MAC\":\"" + SAVE.MAC[0] + "," + SAVE.MAC[1] + "," + SAVE.MAC[2] + "," + SAVE.MAC[3] + "," + SAVE.MAC[4] + "," + SAVE.MAC[5] 
      + "\", \"IP\":\"" + SAVE.IP[0] + "," + SAVE.IP[1] + "," + SAVE.IP[2] + "," + SAVE.IP[3] + "\", \"GATEWAY\":\"" + SAVE.GATEWAY[0] + "," + SAVE.GATEWAY[1] + "," + SAVE.GATEWAY[2] + "," + SAVE.GATEWAY[3] + "\", \"SUBNET\":\"" + SAVE.SUBNET[0] + "," + SAVE.SUBNET[1] + "," + SAVE.SUBNET[2] + "," + SAVE.SUBNET[3]  
      + "\", \"DNS\":\"" + SAVE.DNS[0] + "," + SAVE.DNS[1] + "," + SAVE.DNS[2] + "," + SAVE.DNS[3] + "\", \"PORT\":\"" + SAVE.PORT + "\"}");
    });                                                                                                   //Данные о настройках сети
    SERVER.on(UriBraces("/setting/{}/{}/{}/{}/{}/{}/{}/{}/{}"), [](){
      memset(SAVE.SSID, 0, (sizeof(SAVE.SSID)/sizeof(char)) - 1);
      strncpy(SAVE.SSID, SERVER.pathArg(0).c_str(), SERVER.pathArg(0).length());
      memset(SAVE.PASSWORD, 0, (sizeof(SAVE.PASSWORD)/sizeof(char)) - 1);
      strncpy(SAVE.PASSWORD, SERVER.pathArg(1).c_str(), SERVER.pathArg(1).length());

      SAVE.CONNECTED_COUNT  = strtoul(SERVER.pathArg(2).c_str(), NULL, 0);                                //Количество попыток подключения
  
      SAVE.MAC[0]           = TO.SPLIT(SERVER.pathArg(3), 0);                                             //MAC адрес
      SAVE.MAC[1]           = TO.SPLIT(SERVER.pathArg(3), 1);
      SAVE.MAC[2]           = TO.SPLIT(SERVER.pathArg(3), 2);
      SAVE.MAC[3]           = TO.SPLIT(SERVER.pathArg(3), 3);
      SAVE.MAC[4]           = TO.SPLIT(SERVER.pathArg(3), 4);
      SAVE.MAC[5]           = TO.SPLIT(SERVER.pathArg(3), 5);
      
      SAVE.IP[0]            = TO.SPLIT(SERVER.pathArg(4), 0);                                             //IP адрес
      SAVE.IP[1]            = TO.SPLIT(SERVER.pathArg(4), 1);
      SAVE.IP[2]            = TO.SPLIT(SERVER.pathArg(4), 2);
      SAVE.IP[3]            = TO.SPLIT(SERVER.pathArg(4), 3);
  
      SAVE.GATEWAY[0]       = TO.SPLIT(SERVER.pathArg(5), 0);                                             //Шлюз
      SAVE.GATEWAY[1]       = TO.SPLIT(SERVER.pathArg(5), 1);
      SAVE.GATEWAY[2]       = TO.SPLIT(SERVER.pathArg(5), 2);
      SAVE.GATEWAY[3]       = TO.SPLIT(SERVER.pathArg(5), 3);
  
      SAVE.SUBNET[0]        = TO.SPLIT(SERVER.pathArg(6), 0);                                             //Маска сети
      SAVE.SUBNET[1]        = TO.SPLIT(SERVER.pathArg(6), 1);
      SAVE.SUBNET[2]        = TO.SPLIT(SERVER.pathArg(6), 2);
      SAVE.SUBNET[3]        = TO.SPLIT(SERVER.pathArg(6), 3);
  
      SAVE.DNS[0]           = TO.SPLIT(SERVER.pathArg(7), 0);                                             //DNS адрес
      SAVE.DNS[1]           = TO.SPLIT(SERVER.pathArg(7), 1);
      SAVE.DNS[2]           = TO.SPLIT(SERVER.pathArg(7), 2);
      SAVE.DNS[3]           = TO.SPLIT(SERVER.pathArg(7), 3);
      
      SAVE.PORT             = strtoul(SERVER.pathArg(8).c_str(), NULL, 0);
      
      EEPROM_SAVE();
      
      SERVER.send(200, "application/json; charset=utf-8", "OK");
      delay(500);
      ESP.restart();
    });                                                                                                   //Сохраняет настройки вольтамперметра и перезапускает его
    
    SERVER.onNotFound([](){
      SERVER.send(404, "text/plain", "404: Not found");                                                   //Страница не найдена
    });
    SERVER.begin();
  }
  TFT.setTextColor(TFT_GREEN, TFT_BLACK);
  TFT.drawString(OK, TFT.width()-15, 150, FONT2);                                                         //Сервер успешно запущен
  
  SAVE.IS_TOUCH = 0;
  EEPROM_SAVE();
  DISPLAY_STATIC();
}

/*
 * Калибруем сенсорный экран
 */
void TOUCH_CALIBRATE()                                                                                    
{
  TFT.fillScreen(TFT_BLACK);
  TFT.setCursor(20, 0);
  TFT.setTextFont(2);
  TFT.setTextSize(1);
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);

  TFT.println(F("Touch corners as indicated"));

  TFT.setTextFont(1);
  TFT.println();

  TFT.calibrateTouch(SAVE.CAL_DATA, TFT_MAGENTA, TFT_BLACK, 15);

  TFT.fillScreen(TFT_BLACK);
  SAVE.IS_TOUCH = 0;
  TFT.setTextColor(TFT_GREEN, TFT_BLACK);
  EEPROM_SAVE();
  delay(500);
  ESP.restart();
}


void loop() { 
  if (millis() - TIME.TIME_DATA >= 1000)                                                                //Раз в секунду считываем данные с модуля реального времени, если нужно чаще, то Вам потребуется другой модуль, DS1302 при запросах чаще раз в секунду будет отдавать неверную дату и время
  {
    RTC.getDateTime(&NOW);
    DATA.STRING_DATE = TO.DISPLAY_DATE(NOW.day) +"."+ TO.DISPLAY_DATE(NOW.month) +"."+ TO.DISPLAY_DATE(NOW.year) +" "+ TO.DISPLAY_DATE(NOW.hour) +":"+ TO.DISPLAY_DATE(NOW.minute) +":"+ TO.DISPLAY_DATE(NOW.second);
    
    char SERIAL_WRITE[50] =  "";
    if (DATA.FAN < 100)
      strcat(SERIAL_WRITE, DATA.FAN < 10 ? "00" : "0");
    strcat(SERIAL_WRITE, String(DATA.FAN).c_str());
    strcat(SERIAL_WRITE, QC_BUTTON.IS_PRESSED ? "x1" : "x0");
    strcat(SERIAL_WRITE, QI_BUTTON.IS_PRESSED ? "x1" : "x0");
    strcat(SERIAL_WRITE, ST_BUTTON.IS_PRESSED ? "x1" : "x0");
    strcat(SERIAL_WRITE, MT_BUTTON.IS_PRESSED ? "x1" : "x0");
    strcat(SERIAL_WRITE, EL_BUTTON.IS_PRESSED ? "x1" : "x0");
    Serial.println(SERIAL_WRITE);                                                                     //Отправляем команды по UART в зависимости от включенных/выключенных сенсорных кнопок
      
    SENSORS.setWaitForConversion(false);                                                              //Делает запрос асинхронным
    SENSORS.requestTemperaturesByAddress(THERMOMETER);                                                //Отправьте команду, чтобы получить температуру
    SENSORS.setWaitForConversion(true);

    DATA.TEMPERATURE  = SENSORS.getTempC(THERMOMETER);                                                //Получаем температуру
    TIME.TIME_DATA    = millis();
  }
  if (SAVE.IS_WIFI)                                                                                   //Если WI-FI то выполняем команды сервера
    SERVER.handleClient();
  DATA.VOLTS  = INA226.getBusVoltage_V();                                                             //Получаем значение напряжения
  if (DATA.VOLTS < 0)
    DATA.VOLTS = 0;
  DATA.AMPS   = INA226.getCurrent_A() * SET_CORRECTION_FACTOR;                                        //Получаем значение силы тока
  if (DATA.AMPS < 0)
    DATA.AMPS = 0;
  DATA.WATTS  = DATA.VOLTS * DATA.AMPS;                                                               //Получаем значение ватт INA226.getBusPower();

  
  DATA.mAh += DATA.AMPS * (millis() - TIME.SET_MILLISECOND) / 3600;                                   //Расчет емкости  в мАч
  DATA.mWh += DATA.AMPS  * DATA.VOLTS * (millis() - TIME.SET_MILLISECOND) / 3600;                     //Расчет мВтч
  TIME.SET_MILLISECOND = millis();                                                                    //Обновляем текущее время

  if (DATA.TEMPERATURE >= DATA.MINIMUM_TEMPERATURE && DATA.TEMPERATURE <= DATA.MAXIMUM_TEMPERATURE)   //Если температура в среднем показателе, рассчитываем обороты
  {
    DATA.FAN = (DATA.TEMPERATURE - DATA.MINIMUM_TEMPERATURE) * 255 / (DATA.MAXIMUM_TEMPERATURE - DATA.MINIMUM_TEMPERATURE);   //Рассчитываем  ШИМ вентилятора.
    if (DATA.FAN < 85)
      DATA.FAN = 85;
  } 
  else if (DATA.TEMPERATURE < DATA.MINIMUM_TEMPERATURE)                                               //Если температура минимум
  {
    DATA.FAN = 0;                                                                                     //Отключаем вентилятор
  } 
  else if (DATA.TEMPERATURE >= DATA.MAXIMUM_TEMPERATURE)                                              //Если температура максимум
  {
    DATA.FAN = 255;                                                                                   //Включаем вентилятор
  }

  if (millis() - TIME.TIME_GRAPH >= 240)
  {
    DATA.VOLTAGE_GRAPH_SUMM  += DATA.VOLTS;                                                           //Сумируем показания напряжения
    DATA.AMP_GRAPH_SUMM      += DATA.AMPS;                                                            //Сумируем показания силы тока
    DATA.WATTS_GRAPH_SUMM    += DATA.WATTS;                                                           //Сумируем показания силы тока
    if (TIME.SECOND_GRAPH >= (pow(10, SAVE.GRAPH_OPTION) * (SAVE.GRAPH_OPTION == 2 ? 6 : 1)))         //Отсчитываем указанный в настройках период
    {
      DATA.COUNT_ARRAY = sizeof(DATA.VOLTAGE_GRAPH) / sizeof(float);                                  //Получаем длину массива
      if (TIME.MINUT_GRAPH >= DATA.COUNT_ARRAY)                                                       //Если количество минут больше длины массива, то сдвигаем его влево
      {
        for (uint16_t i = 0; i < DATA.COUNT_ARRAY; ++i) 
        {
          DATA.VOLTAGE_GRAPH[i]  = DATA.VOLTAGE_GRAPH[i + 1];
          DATA.AMP_GRAPH[i]      = DATA.AMP_GRAPH[i + 1];
          DATA.WATTS_GRAPH[i]    = DATA.WATTS_GRAPH[i + 1];
        }

        TIME.MINUT_GRAPH -= 1;
      }
      DATA.VOLTAGE_GRAPH[TIME.MINUT_GRAPH]  = DATA.VOLTAGE_GRAPH_SUMM / (pow(10, SAVE.GRAPH_OPTION) * (SAVE.GRAPH_OPTION == 2 ? 6 : 1));  //Посчитываем среднее арифметическое значение показаний напряжения
      DATA.AMP_GRAPH[TIME.MINUT_GRAPH]      = DATA.AMP_GRAPH_SUMM / (pow(10, SAVE.GRAPH_OPTION) * (SAVE.GRAPH_OPTION == 2 ? 6 : 1));      //Посчитываем среднее арифметическое значение показаний силы тока
      DATA.WATTS_GRAPH[TIME.MINUT_GRAPH]    = DATA.WATTS_GRAPH_SUMM / (pow(10, SAVE.GRAPH_OPTION) * (SAVE.GRAPH_OPTION == 2 ? 6 : 1));    //Посчитываем среднее арифметическое значение показаний ватт
      TIME.SECOND_GRAPH                     = 0;
      DATA.VOLTAGE_GRAPH_SUMM               = 0;
      DATA.AMP_GRAPH_SUMM                   = 0;
      DATA.WATTS_GRAPH_SUMM                 = 0;
      ++TIME.MINUT_GRAPH;
    }
    ++TIME.SECOND_GRAPH;
    TIME.TIME_GRAPH = millis();
  }


  if (SAVE.SD_WRITE)                                                                              //Если в настройках разрешено сохранять в лог файл сохранить.
  {
    if (millis() - TIME.TIME_LOG >= 1000)
    {
      DATA.VOLTS_LOG          += DATA.VOLTS;                                                      //Сумируем показания напряжения
      DATA.AMPS_LOG           += DATA.AMPS;                                                       //Сумируем показания силы тока
      DATA.WATTS_LOG          += DATA.WATTS;                                                      //Сумируем показания ватт
      DATA.TEMPERATURES_LOG   += DATA.TEMPERATURE;                                                //Сумируем показания температуры
      if (TIME.SECOND_LOG >= 15)                                                                  //Отсчитываем 15 секунд
      {
        fs::File FILE; 
        FILE = SD.open("/DATA/" + TO.DISPLAY_DATE(NOW.day) +"_"+ TO.DISPLAY_DATE(NOW.month) +"_"+ TO.DISPLAY_DATE(NOW.year) +"_"+ TO.DISPLAY_DATE(NOW.hour) + "_00_00" + ".csv", FILE_WRITE);                     //Создаём файл для записи значений или открываем его если такой уже есть
        if (FILE.size() == 0)
          FILE.println(F("DATE;VOLT;AMP;WATT;TEMPERATURE;")); 
        FILE.print(TO.DISPLAY_DATE(NOW.day) +"."+ TO.DISPLAY_DATE(NOW.month) +"."+ TO.DISPLAY_DATE(NOW.year) +"_"+ TO.DISPLAY_DATE(NOW.hour) +":"+ TO.DISPLAY_DATE(NOW.minute) +":"+ TO.DISPLAY_DATE(NOW.second)); //Записываем данные в файл
        FILE.print(";");
        FILE.print(DATA.VOLTS_LOG / 15);
        FILE.print(";");
        FILE.print(DATA.AMPS_LOG / 15);
        FILE.print(";");
        FILE.print(DATA.WATTS_LOG / 15);
        FILE.print(";");
        FILE.println(DATA.TEMPERATURES_LOG / 15);
        FILE.close();                                                                             //Закрываем файл для записи и обнуляем вспомогательные переменные
        DATA.VOLTS_LOG                              = 0;
        DATA.AMPS_LOG                               = 0;
        DATA.WATTS_LOG                              = 0;
        DATA.TEMPERATURES_LOG                       = 0;
        TIME.SECOND_LOG                             = 0;
      }
      ++TIME.SECOND_LOG;
      TIME.TIME_LOG                                 = millis();
    }
  }

  if (millis() - TIME.TIME_GRAPH_FAST >= 10)                                                      //Считываем данные раз в 10 миллисекунд
  {
    DATA.COUNT_ARRAY_FAST = sizeof(DATA.VOLTAGE_GRAPH_FAST) / sizeof(float);                      //Получаем длину массива
    if (TIME.TEAK >= DATA.COUNT_ARRAY_FAST)                                                       //Если количество минут больше длины массива, то сдвигаем его влево
    {
      for (uint16_t i = 0; i < DATA.COUNT_ARRAY_FAST; ++i) 
      {
        DATA.VOLTAGE_GRAPH_FAST[i]      = DATA.VOLTAGE_GRAPH_FAST[i + 1];
        DATA.AMP_GRAPH_FAST[i]          = DATA.AMP_GRAPH_FAST[i + 1];
        DATA.WATTS_GRAPH_FAST[i]        = DATA.WATTS_GRAPH_FAST[i + 1];
      }
      TIME.TEAK                         -= 1;
    }

    DATA.VOLTAGE_GRAPH_FAST[TIME.TEAK]  = DATA.VOLTS;                                             //Посчитываем среднее арифметическое показаний
    DATA.AMP_GRAPH_FAST[TIME.TEAK]      = DATA.AMPS;
    DATA.WATTS_GRAPH_FAST[TIME.TEAK]    = DATA.WATTS;
    ++TIME.TEAK;
    TIME.TIME_GRAPH_FAST                = millis();
  }

  uint16_t x = 0, y = 0;
  bool IS_PRESSED = false;
  if (millis() - TIME.TIME_BUTTOM >= 400)
  {
    IS_PRESSED = TFT.getTouch(&x, &y);                                                            //Если произошло событие нажатие на экран, то передаём координаты точки касания
    if (IS_PRESSED) {
      if (DATA.DISPLAY_NUMBER != 3)
      {
        if (WIFI_BUTTON.IS_PRESS(x, y))                                                           //Кнопка WI-FI
        {
          TFT.pushImage(245, 0, 30, 30, WIFI_BUTTON.IS_PRESSED ? wifi_on : wifi_off);
          SAVE.IS_WIFI = WIFI_BUTTON.IS_PRESSED;
          EEPROM_SAVE();
          ESP.restart();
        }
  
        if (MENU_BUTTON.IS_PRESS(x, y))                                                             //Кнопка страницы меню
        {
          if (DATA.DISPLAY_NUMBER == 2)
            DATA.DISPLAY_NUMBER = 0;
          else
          {
            DATA.DISPLAY_NUMBER = 2;
            DATA.IS_SIZE        = true;
          }
          DISPLAY_STATIC();
        }
        
        if (GRAPH_BUTTON.IS_PRESS(x, y))                                                            //Кнопка страницы графика
        {
          if (DATA.DISPLAY_NUMBER == 1 || DATA.DISPLAY_NUMBER == 2)
            DATA.DISPLAY_NUMBER = 0;
          else
            DATA.DISPLAY_NUMBER = 1;
          DISPLAY_STATIC();
        }
      }        
      if (DATA.DISPLAY_NUMBER == 0)                                                                 //Инициализация кнопок управления силовой нагрузкой
      {
        QC_BUTTON.IS_PRESS(x, y);
        QI_BUTTON.IS_PRESS(x, y);
        ST_BUTTON.IS_PRESS(x, y);
        MT_BUTTON.IS_PRESS(x, y);
        EL_BUTTON.IS_PRESS(x, y);
        
        if (x >= 175 && x <= 315 && y >= 90 && y <= 170)
        {
          DATA.DISPLAY_NUMBER = 1;
          DISPLAY_STATIC();
        }
      }
      if (DATA.DISPLAY_NUMBER == 1)                                                               //Инициализация кнопок управления графиком
      {
        if (x >= 45 && x <= 165 && y >= 15 && y <= 30)
        {
          if (x >= 45 && x <= 84 && y >= 15 && y <= 30)
            SAVE.GRAPH_OPTION = 0;
          if (x >= 85 && x <= 129 && y >= 15 && y <= 30)
            SAVE.GRAPH_OPTION = 1;
          if (x >= 130 && x <= 165 && y >= 15 && y <= 30)
            SAVE.GRAPH_OPTION = 2;
          memset(DATA.VOLTAGE_GRAPH, 0, (sizeof(DATA.VOLTAGE_GRAPH)/sizeof(float)) - 1);
          memset(DATA.AMP_GRAPH, 0, (sizeof(DATA.AMP_GRAPH)/sizeof(float)) - 1);
          memset(DATA.WATTS_GRAPH, 0, (sizeof(DATA.AMP_GRAPH)/sizeof(float)) - 1);
          TIME.MINUT_GRAPH = 0;
          DISPLAY_STATIC();
          EEPROM_SAVE();
        }
      }
      if (DATA.DISPLAY_NUMBER == 2)                                                           //Инициализация кнопок управления настройками
      {
        if (!DELETE_BUTTON.IS_PRESSED)                                                        //Кнопка очистки карты SD
          DELETE_BUTTON.IS_PRESS(x, y);
        else
        {
          DELETE_BUTTON.IS_PRESSED = !NO_BUTTON.IS_PRESS(x, y);
          if (YES_BUTTON.IS_PRESS(x, y))
          {
            CLEAR_DIRECTORY();
            DELETE_BUTTON.IS_PRESSED  = false;
            DISPLAY_SIZE_DIRECTORY();
          }
        }
        
        if (OFF_ON_BUTTON.IS_PRESS(x, y))                                                     //Кнопка включения/отключения записи в лог файл
        {
          SAVE.SD_WRITE = OFF_ON_BUTTON.IS_PRESSED;
          EEPROM_SAVE();
        }
                                                                                              //Выбор цвета данных по напряжению, силы тока, ватт, графика
        if (VOLTAGE_BUTTON.IS_PRESS(x, y) || AMP_BUTTON.IS_PRESS(x, y) || WATTS_BUTTON.IS_PRESS(x, y) || GRID_GRAPHICS_BUTTON.IS_PRESS(x, y))
        {
          DATA.DISPLAY_NUMBER = 3;
          IS_PRESSED          = false;
          DISPLAY_STATIC();
        }
      }
      TIME.TIME_BUTTOM  = millis();
    }
  }

  if (DATA.DISPLAY_NUMBER == 0)                                                             //Выбрана главная страница
    DISPLAY_DATA();
  if (DATA.DISPLAY_NUMBER == 1)                                                             //Выбрана страница графиков
    DISPLAY_GRAPH();
  if (DATA.DISPLAY_NUMBER == 2)                                                             //Выбрана страница настроек
    DISPLAY_SETTING();
  if (DATA.DISPLAY_NUMBER == 3)                                                             //Выбрана страница выбора цвета данных
    DISPLAY_COLOR(x, y, IS_PRESSED);
    
}



/*
 * Функция DISPLAY_STATIC выводит статистические данные на экран, чтобы снизить нагрузку на ESP8266
*/
void DISPLAY_STATIC()
{
  TFT.fillScreen(TFT_BLACK);
  
  if (DATA.DISPLAY_NUMBER != 3)                                                                                             //Выводить данные если не открыто окно с выбором цвета
  {
    TFT.setTextColor(TFT_WHITE, TFT_BLACK);
    TFT.drawString(F("DATE:"), 0, 0, FONT2);

    TFT.pushImage(290, 0, 30, 30, menu);
    TFT.pushImage(245, 0, 30, 30, WIFI_BUTTON.IS_PRESSED ? wifi_on : wifi_off);
  }
  if (DATA.DISPLAY_NUMBER == 0)                                                                                             //Выводит данные для экрана с показаниями
  {
    TFT.pushImage(200, 0, 30, 30, graph);
    TFT.drawString(F("IP:"), 0, 15, FONT2);

    TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.VOLTAGE_COLOR[0], SAVE.VOLTAGE_COLOR[1], SAVE.VOLTAGE_COLOR[2]), TFT_BLACK);
    TFT.drawString(F("V"), 125, 50, FONT4);
  
    TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.AMP_COLOR[0], SAVE.AMP_COLOR[1], SAVE.AMP_COLOR[2]), TFT_BLACK);
    TFT.drawString(F("A"), 125, 100, FONT4);
  
    TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.WATTS_COLOR[0], SAVE.WATTS_COLOR[1], SAVE.WATTS_COLOR[2]), TFT_BLACK);
    TFT.drawString(F("W"), 125, 150, FONT4);
  }
  if (DATA.DISPLAY_NUMBER == 1)                                                                                             //Выводит данные для графика
  {
    TFT.pushImage(200, 0, 30, 30, voltage);
    TFT.drawString(F("OPTION:"), 0, 15, FONT2);
  }
  if (DATA.DISPLAY_NUMBER == 2)                                                                                             //Выводит данные для экрана настроек
  {
    TFT.pushImage(200, 0, 30, 30, voltage);
    TFT.drawString(F("IP:"), 0, 15, FONT2);

    TFT.drawString(F("CARD SD SIZE:"), 0, 40, FONT2);
    TFT.drawString(F("DELETE ALL DATA:"), 0, 80, FONT2);
    TFT.drawString(F("SAVE DATA TO SD:"), 0, 130, FONT2);

    TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.VOLTAGE_COLOR[0], SAVE.VOLTAGE_COLOR[1], SAVE.VOLTAGE_COLOR[2]), TFT_BLACK);
    TFT.drawString(F("VOLTAGE............."), 0, 175, FONT2);
    TFT.fillRect(TFT.width() - 200, 158, 30, 30, TO.RGB_TO_RGB16(SAVE.VOLTAGE_COLOR[0], SAVE.VOLTAGE_COLOR[1], SAVE.VOLTAGE_COLOR[2]));
    TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.AMP_COLOR[0], SAVE.AMP_COLOR[1], SAVE.AMP_COLOR[2]), TFT_BLACK);
    TFT.drawString(F("AMP.................."), 170, 175, FONT2);
    TFT.fillRect(TFT.width() - 34, 158, 30, 30, TO.RGB_TO_RGB16(SAVE.AMP_COLOR[0], SAVE.AMP_COLOR[1], SAVE.AMP_COLOR[2]));
    
    TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.WATTS_COLOR[0], SAVE.WATTS_COLOR[1], SAVE.WATTS_COLOR[2]), TFT_BLACK);
    TFT.drawString(F("WATTS................"), 0, 225, FONT2);
    TFT.fillRect(TFT.width() - 200, 208, 30, 30, TO.RGB_TO_RGB16(SAVE.WATTS_COLOR[0], SAVE.WATTS_COLOR[1], SAVE.WATTS_COLOR[2]));
    TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]), TFT_BLACK);
    TFT.drawString(F("GRID GRAPHICS...."), 170, 225, FONT2);
    TFT.fillRect(TFT.width() - 34, 208, 30, 30, TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]));
  }
}


/*
 * Выводит показания на экран
 */
void DISPLAY_DATA()
{
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);  
  TFT.drawString(DATA.STRING_DATE, 50, 0, FONT2);
  TFT.drawString(DATA.STRING_IP, 50, 15, FONT2);

  TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.VOLTAGE_COLOR[0], SAVE.VOLTAGE_COLOR[1], SAVE.VOLTAGE_COLOR[2]), TFT_BLACK);                            //Устанавливаем цвет текста и фона
  TFT.drawString(TO.DISPLAY_TEXT(DATA.VOLTS, 5, DATA.VOLTS < 10 ? 3 : 2), -12, 33, FONT6);                                                      //Вывородит значение напряжения
  
  TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.AMP_COLOR[0], SAVE.AMP_COLOR[1], SAVE.AMP_COLOR[2]), TFT_BLACK);                                        //Устанавливаем цвет текста и фона
  TFT.drawString(TO.DISPLAY_TEXT(DATA.AMPS, 5, DATA.AMPS < 10 ? 3 : 2), -12, 83, FONT6);                                                        //Вывородит значение силы тока

  TFT.setTextColor(TO.RGB_TO_RGB16(SAVE.WATTS_COLOR[0], SAVE.WATTS_COLOR[1], SAVE.WATTS_COLOR[2]), TFT_BLACK);                                  //Устанавливаем цвет текста и фона
  TFT.drawString(TO.DISPLAY_TEXT(DATA.WATTS, 5, DATA.WATTS < 10 ? 3 : DATA.WATTS < 100 ? 2 : DATA.WATTS < 1000 ? 1 : 0), -12, 133, FONT6);      //Вывородит значение ватт

  TFT.drawString(DATA.mAh >= 10000 ? TO.DISPLAY_TEXT((DATA.mAh / 1000), 8, DATA.mAh < 100000 ? 3 : DATA.mAh < 1000000 ? 2 : DATA.mAh < 10000000 ? 1 : 0) : TO.DISPLAY_TEXT(DATA.mAh, DATA.mAh < 10 ? 12 : DATA.mAh < 100 ? 10 : 7, 2), 0, 180, FONT4);                      //Вывородит значение mAh / Ah
  TFT.drawString(DATA.mAh >= 10000 ? "Ah" : "mAh", 100, 180, FONT4);

  TFT.drawString(DATA.mWh > 10000 ? TO.DISPLAY_TEXT((DATA.mWh / 1000), 6, DATA.mWh < 100000 ? 3 : DATA.mWh < 1000000 ? 2 : DATA.mWh < 10000000 ? 1 : 0) : TO.DISPLAY_TEXT(DATA.mWh, DATA.mWh < 10 ? 10 : DATA.mWh < 100 ? 8 : 6, DATA.mWh < 1000 ? 2 : 1), 175, 180, FONT4); //Вывородит значение mWh / Wh
  TFT.drawString(DATA.mWh > 10000 ? "Wh" : "mWh", DATA.mWh > 10000 ? 270 : 260, 180, FONT4);
  
  TFT.setTextColor(0x07BF, TFT_BLACK);
  TFT.drawString("t: " + String(TO.DISPLAY_TEXT(DATA.TEMPERATURE, 6, 1)) + " C`    ", 175, 33, FONT4);                                          //Вывородит значение температуры
  TFT.drawString("fan: " + String(TO.DISPLAY_TEXT(DATA.FAN / 2.55, 3, 0)) + " %    ", 175, 63, FONT4);                                          //Вывородит значение оборотов вентилятора в процентах

  DATA.MAX_GRAPH = 0;
  DATA.COUNT_ARRAY_FAST = sizeof(DATA.VOLTAGE_GRAPH_FAST) / sizeof(float);                                                                      //Получаем длину массива
  for(uint16_t i = 0; i < (TIME.TEAK < DATA.COUNT_ARRAY_FAST ? TIME.TEAK : DATA.COUNT_ARRAY_FAST); ++i)                                         //Находим максимальное значение для построения шкалы графика
  {
    DATA.MAX_GRAPH = max(DATA.MAX_GRAPH, uint8_t(ceil(DATA.VOLTAGE_GRAPH_FAST[TIME.TEAK <= DATA.COUNT_ARRAY_FAST ? i : i + TIME.TEAK - DATA.COUNT_ARRAY_FAST]))); 
    DATA.MAX_GRAPH = max(DATA.MAX_GRAPH, uint8_t(ceil(DATA.AMP_GRAPH_FAST[TIME.TEAK <= DATA.COUNT_ARRAY_FAST ? i : i + TIME.TEAK - DATA.COUNT_ARRAY_FAST])));     
    DATA.MAX_GRAPH = max(DATA.MAX_GRAPH, uint8_t(ceil(DATA.WATTS_GRAPH_FAST[TIME.TEAK <= DATA.COUNT_ARRAY_FAST ? i : i + TIME.TEAK - DATA.COUNT_ARRAY_FAST])));   
  }
  if (DATA.MAX_GRAPH == 0)
    DATA.MAX_GRAPH = 1;
  
  if (DATA.MAX_GRAPH_OLD != DATA.MAX_GRAPH)                                                                                                      //Очищаем экран если значения графика больше чем значения на шкале графика 
    TFT.fillRect(180, 90, 140, 80, TFT_BLACK);
  DATA.MAX_GRAPH_OLD =DATA. MAX_GRAPH;
  
  for(uint16_t i = 0; i < (TIME.TEAK < 140 ? TIME.TEAK : 140); ++i)
  {
                                                                                                                                                  //Строим график напряжения
    TFT.drawLine(i+175, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH_FAST[TIME.TEAK <= 140 ? i : i + TIME.TEAK - 140], 0, DATA.MAX_GRAPH, 170, 90), i+175, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH_FAST[i == 0 ? i : TIME.TEAK <= 140 ? i - 2 :  i + TIME.TEAK - 142], 0, DATA.MAX_GRAPH, 170, 90), TFT_BLACK);
    TFT.drawLine(i+175, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH_FAST[TIME.TEAK <= 140 ? i : i + TIME.TEAK - 140], 0, DATA.MAX_GRAPH, 170, 90), i+175, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH_FAST[i == 0 ? i : TIME.TEAK <= 140 ? i - 1 : i + TIME.TEAK - 141], 0, DATA.MAX_GRAPH, 170, 90), TO.RGB_TO_RGB16(SAVE.VOLTAGE_COLOR[0], SAVE.VOLTAGE_COLOR[1], SAVE.VOLTAGE_COLOR[2]));

                                                                                                                                                  //Строим график силы тока
    TFT.drawLine(i+175, TO.MAPFLOAT(DATA.AMP_GRAPH_FAST[TIME.TEAK <= 140 ? i : i + TIME.TEAK - 140], 0, DATA.MAX_GRAPH, 170, 90), i+175, TO.MAPFLOAT(DATA.AMP_GRAPH_FAST[i == 0 ? i : TIME.TEAK <= 140 ? i - 2 :  i + TIME.TEAK - 142], 0, DATA.MAX_GRAPH, 170, 90), TFT_BLACK);
    TFT.drawLine(i+175, TO.MAPFLOAT(DATA.AMP_GRAPH_FAST[TIME.TEAK <= 140 ? i : i + TIME.TEAK - 140], 0, DATA.MAX_GRAPH, 170, 90), i+175, TO.MAPFLOAT(DATA.AMP_GRAPH_FAST[i == 0 ? i : TIME.TEAK <= 140 ? i - 1 : i + TIME.TEAK - 141], 0, DATA.MAX_GRAPH, 170, 90), TO.RGB_TO_RGB16(SAVE.AMP_COLOR[0], SAVE.AMP_COLOR[1], SAVE.AMP_COLOR[2]));

                                                                                                                                                  //Строим график силы ватт
    TFT.drawLine(i+175, TO.MAPFLOAT(DATA.WATTS_GRAPH_FAST[TIME.TEAK <= 140 ? i : i + TIME.TEAK - 140], 0, DATA.MAX_GRAPH, 170, 90), i+175, TO.MAPFLOAT(DATA.WATTS_GRAPH_FAST[i == 0 ? i : TIME.TEAK <= 140 ? i - 2 :  i + TIME.TEAK - 142], 0, DATA.MAX_GRAPH, 170, 90), TFT_BLACK);
    TFT.drawLine(i+175, TO.MAPFLOAT(DATA.WATTS_GRAPH_FAST[TIME.TEAK <= 140 ? i : i + TIME.TEAK - 140], 0, DATA.MAX_GRAPH, 170, 90), i+175, TO.MAPFLOAT(DATA.WATTS_GRAPH_FAST[i == 0 ? i : TIME.TEAK <= 140 ? i - 1 : i + TIME.TEAK - 141], 0, DATA.MAX_GRAPH, 170, 90), TO.RGB_TO_RGB16(SAVE.WATTS_COLOR[0], SAVE.WATTS_COLOR[1], SAVE.WATTS_COLOR[2]));
  }

  TFT.drawLine(175, 171, 315, 171, TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]));
  TFT.drawLine(175, 90, 175, 170, TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]));
  
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  TFT.pushImage(0, 210, 30, 30, QC_BUTTON.IS_PRESSED ? qc_on : qc_off);
  TFT.pushImage(40, 210, 30, 30, QI_BUTTON.IS_PRESSED ? qi_on : qi_off);
  TFT.pushImage(80, 210, 30, 30, ST_BUTTON.IS_PRESSED ? st_on : st_off);
  TFT.pushImage(120, 210, 30, 30, MT_BUTTON.IS_PRESSED ? mt_on : mt_off);
  TFT.pushImage(160, 210, 30, 30, EL_BUTTON.IS_PRESSED ? el_on : el_off);
}



/*
 * Выводин график на экран
 */
void DISPLAY_GRAPH()
{  
  TFT.setTextColor(TFT_GREEN, TFT_BLACK);
  if (SAVE.GRAPH_OPTION == 0)                                                                                         //Отображаем режим который выбран на текущий момент
    TFT.drawString("[sec]   min    hr ", 50, 15, FONT2);
  if (SAVE.GRAPH_OPTION == 1)
    TFT.drawString(" sec   [min]   hr ", 50, 15, FONT2);
  if (SAVE.GRAPH_OPTION == 2)
    TFT.drawString(" sec    min   [hr] ", 50, 15, FONT2);
  
  
  DATA.MAX_GRAPH = 0;                                                                                                 //Находим максимальное значение среди данных графика
  DATA.COUNT_ARRAY = sizeof(DATA.VOLTAGE_GRAPH) / sizeof(float);                                                      //Получаем длину массива 
  for(uint16_t i = 0; i < (TIME.MINUT_GRAPH < DATA.COUNT_ARRAY ? TIME.MINUT_GRAPH : DATA.COUNT_ARRAY); ++i)
  {
    DATA.MAX_GRAPH = max(DATA.MAX_GRAPH, uint8_t(ceil(DATA.VOLTAGE_GRAPH[TIME.MINUT_GRAPH <= DATA.COUNT_ARRAY ? i : i + TIME.MINUT_GRAPH - DATA.COUNT_ARRAY])));
    DATA.MAX_GRAPH = max(DATA.MAX_GRAPH, uint8_t(ceil(DATA.AMP_GRAPH[TIME.MINUT_GRAPH <= DATA.COUNT_ARRAY ? i : i + TIME.MINUT_GRAPH - DATA.COUNT_ARRAY])));
    DATA.MAX_GRAPH = max(DATA.MAX_GRAPH, uint8_t(ceil(DATA.WATTS_GRAPH[TIME.MINUT_GRAPH <= DATA.COUNT_ARRAY ? i : i + TIME.MINUT_GRAPH - DATA.COUNT_ARRAY])));
  }
  if (DATA.MAX_GRAPH == 0)
       DATA.MAX_GRAPH = 1;
       
  if (DATA.MAX_GRAPH_OLD != DATA.MAX_GRAPH && DATA.MAX_GRAPH_OLD != 0)
    DISPLAY_STATIC();
  DATA.MAX_GRAPH_OLD = DATA.MAX_GRAPH;

  TFT.setTextColor(TFT_WHITE, TFT_BLACK);
  
  TFT.drawString(DATA.STRING_DATE, 50, 0, FONT2);
  
  for (uint16_t x = 33; x <= DATA.WIDGHT; x += 5) {                                                                     //Строим сетку графика
    if (x - 1 <= DATA.WIDGHT - 10)
      for (uint8_t y = 60; y < DATA.HEIGHT_GRAPH; y += 20) {
        TFT.drawFastHLine(x, y, 2, TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]));
      }

    if ((x - 33) % 25 == 0) {
      TFT.drawFastHLine(x - 33 == 0 ? x + 1 : x == DATA.WIDGHT - 12 ? x - 2 : x - 1, 40, x - 33 == 0  ? 2 : 3, TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]));
      TFT.drawFastHLine(x - 33 == 0 ? x + 1: x == DATA.WIDGHT - 12 ? x - 2 : x - 1, DATA.HEIGHT_GRAPH, x - 33 == 0 ? 2 : 3, TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]));
    }
  }
  
  for (uint16_t x = 33; x <= DATA.WIDGHT; x += 25) {
    for (uint8_t y = 40; y < DATA.HEIGHT_GRAPH; y += 5) {
      TFT.drawFastVLine(x, y, 2, TO.RGB_TO_RGB16(SAVE.GRID_GRAPHICS_COLOR[0], SAVE.GRID_GRAPHICS_COLOR[1], SAVE.GRID_GRAPHICS_COLOR[2]));
    }
  }

  for (uint8_t y = 42; y < DATA.HEIGHT_GRAPH; y += 5) {
    TFT.drawFastVLine(33, y, 3, 0x0000);
  }

  for(uint16_t i = 0; i < (TIME.MINUT_GRAPH < 276 ? TIME.MINUT_GRAPH : 276); ++i)
  {
                                                                                                                          //Строим график напряжения
    TFT.drawLine(i+33, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH[TIME.MINUT_GRAPH <= 276 ? i : i + TIME.MINUT_GRAPH - 276], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), i+33, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH[i == 0 ? i : TIME.MINUT_GRAPH <= 276 ? i - 2 :  i + TIME.MINUT_GRAPH - 278], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), TFT_BLACK);
    TFT.drawLine(i+33, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH[TIME.MINUT_GRAPH <= 276 ? i : i + TIME.MINUT_GRAPH - 276], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), i+33, TO.MAPFLOAT(DATA.VOLTAGE_GRAPH[i == 0 ? i : TIME.MINUT_GRAPH <= 276 ? i - 1 : i + TIME.MINUT_GRAPH - 277], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), TO.RGB_TO_RGB16(SAVE.VOLTAGE_COLOR[0], SAVE.VOLTAGE_COLOR[1], SAVE.VOLTAGE_COLOR[2]));

                                                                                                                          //Строим график силы тока
    TFT.drawLine(i+33, TO.MAPFLOAT(DATA.AMP_GRAPH[TIME.MINUT_GRAPH <= 276 ? i : i + TIME.MINUT_GRAPH - 276], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), i+33, TO.MAPFLOAT(DATA.AMP_GRAPH[i == 0 ? i : TIME.MINUT_GRAPH <= 276 ? i - 2 :  i + TIME.MINUT_GRAPH - 278], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), TFT_BLACK);
    TFT.drawLine(i+33, TO.MAPFLOAT(DATA.AMP_GRAPH[TIME.MINUT_GRAPH <= 276 ? i : i + TIME.MINUT_GRAPH - 276], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), i+33, TO.MAPFLOAT(DATA.AMP_GRAPH[i == 0 ? i : TIME.MINUT_GRAPH <= 276 ? i - 1 : i + TIME.MINUT_GRAPH - 277], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), TO.RGB_TO_RGB16(SAVE.AMP_COLOR[0], SAVE.AMP_COLOR[1], SAVE.AMP_COLOR[2]));

                                                                                                                          //Строим график силы ватт
    TFT.drawLine(i+33, TO.MAPFLOAT(DATA.WATTS_GRAPH[TIME.MINUT_GRAPH <= 276 ? i : i + TIME.MINUT_GRAPH - 276], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), i+33, TO.MAPFLOAT(DATA.WATTS_GRAPH[i == 0 ? i : TIME.MINUT_GRAPH <= 276 ? i - 2 :  i + TIME.MINUT_GRAPH - 278], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), TFT_BLACK);
    TFT.drawLine(i+33, TO.MAPFLOAT(DATA.WATTS_GRAPH[TIME.MINUT_GRAPH <= 276 ? i : i + TIME.MINUT_GRAPH - 276], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), i+33, TO.MAPFLOAT(DATA.WATTS_GRAPH[i == 0 ? i : TIME.MINUT_GRAPH <= 276 ? i - 1 : i + TIME.MINUT_GRAPH - 277], 0, DATA.MAX_GRAPH, DATA.HEIGHT_GRAPH, 40), TO.RGB_TO_RGB16(SAVE.WATTS_COLOR[0], SAVE.WATTS_COLOR[1], SAVE.WATTS_COLOR[2]));
  }

  uint8_t DIVIDER = 9;
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);                                                                                 //цвет текста белый, цвет заливки текста чёрный
  for (uint8_t y = 40; y <= DATA.HEIGHT_GRAPH; y += 20) {                                                                 //"Текст", положение по оси Х, положение по оси Y, размер шрифта
    TFT.drawString(DIVIDER == 0 ? "   0" : TO.DISPLAY_TEXT(DATA.MAX_GRAPH / 9.0f * DIVIDER, 3, (DATA.MAX_GRAPH / 9 * DIVIDER) >= 10 ? 0 : 1), 0, y > 0 ? y == DATA.HEIGHT_GRAPH ? y + 1 : y - 3 : y, 1);  
    --DIVIDER;
  }
  
  for (uint16_t x = 33; x <= DATA.WIDGHT; x += 25)
    if (x - 33 > 0)                                                                                                       //"Текст", положение по оси Х, положение по оси Y, размер шрифта
      TFT.drawRightString(TO.DISPLAY_TEXT((x - 33) * 0.24 * (SAVE.GRAPH_OPTION == 2 ? 6 : 1) * pow(10, SAVE.GRAPH_OPTION) / pow(60, SAVE.GRAPH_OPTION), 3, 0), (x - 33) * 0.24 * (SAVE.GRAPH_OPTION == 2 ? 600 : 1) * pow(10, SAVE.GRAPH_OPTION) / pow(60, SAVE.GRAPH_OPTION) < 10 ? x + 3 : x + 7, DATA.HEIGHT - 15, 1);
}


/*
 * Выводит настройки на экран
 */
void DISPLAY_SETTING()
{
  TFT.setTextColor(TFT_WHITE, TFT_BLACK);

  TFT.drawString(DATA.STRING_DATE, 50, 0, FONT2);
  TFT.drawString(DATA.STRING_IP, 50, 15, FONT2);
  
  if (!DELETE_BUTTON.IS_PRESSED)
    TFT.pushImage(TFT.width() - 160, 65, 160, 30, DELETE);
  else
  {
    TFT.pushImage(TFT.width() - 160, 65, 76, 30, NO);
    TFT.fillRect(TFT.width() - 84, 65, 3, 30, TFT_BLACK);
    TFT.pushImage(TFT.width() - 80, 65, 76, 30, YES);
  }
  

  TFT.pushImage(TFT.width() - 80, 114, 76, 30, SAVE.SD_WRITE ? ON : OFF);

  
  if (DATA.IS_SIZE)
    DISPLAY_SIZE_DIRECTORY();
}


/*
 * Рассчитываем количество занятого места на SD карте
 */
void DISPLAY_SIZE_DIRECTORY()
{
  TFT.drawString(F("          ........"), TFT.width() - 100, 40, FONT2);  

  fs::File ROOT = SD.open("/");
  DATA.SIZE = 0;
  SIZE_DIRECTORY(ROOT, 0);
  DATA.IS_SIZE = false;

  String SIZE = (DATA.SIZE / 1073741824 > 0 ? String(DATA.SIZE / 1073741824) + F("GB ") + String((DATA.SIZE % 1073741824) / 1048576) + F("MB") : (DATA.SIZE / 1048576 > 0 ? String(DATA.SIZE / 1048576) + F("MB ") + String((DATA.SIZE % 1048576) / 1024) + F("KB") : (DATA.SIZE / 1024 > 0 ? String(DATA.SIZE / 1024) + F("KB ") + String(DATA.SIZE % 1024) + F("B") : String(DATA.SIZE) + F("B"))));
  TFT.drawString(SIZE, TFT.width() - SIZE.length() * 8, 40, FONT2);
}


/*
 * Выводим таблицу цветов на экран
 */
void DISPLAY_COLOR(uint16_t _x, uint16_t _y, bool _IS_PRESSED)
{
  uint8_t COLOR[48][3] = {
                          {128, 0, 0},      {255, 0, 0},      {255, 99, 71},    {210 ,105, 30},   {255, 140, 0},    {184, 134, 11},   {255, 215, 0},    {255, 255, 0}, 
                          {173, 255, 47},   {124, 252, 0},    {0, 255, 0},      {0, 250, 154},    {64, 224, 208},   {0, 255, 255},    {0, 191, 255},    {65, 105, 225}, 
                          {0, 0, 255},      {138, 43, 226},   {148, 0, 211},    {255, 0, 255},    {255, 20, 147},   {255, 105, 80},   {220, 20, 60},    {240, 128, 128}, 
                          {255, 255, 255},  {255, 160, 122},  {139, 69, 19},    {222, 184, 135},  {240, 230, 140},  {107, 142, 35},   {143, 188, 143},  {34, 139, 34},
                          {152, 251, 152},  {60, 179, 113},   {127, 255, 212},  {179, 238, 238},  {176, 224, 230},  {135, 206, 250},  {70, 130, 180},   {176, 196, 222},
                          {100, 149, 237},  {25, 25, 112},    {123, 104, 238},  {75, 0, 130},     {186, 85, 211},   {238, 130 ,238},  {219, 112,147},   {247, 53, 255},
                         };
  uint8_t i = 0;
    for (uint8_t y = 0; y <= 5; ++y)
        for (uint8_t x = 0; x <= 7; ++x)
        {
          TFT.fillRect(x * 30 + 10 * x + 5, y * 30 + 10 * y + 5, 30, 30, TO.RGB_TO_RGB16(COLOR[i][0], COLOR[i][1], COLOR[i][2]));
          if (_IS_PRESSED && (_x >= x * 30 + 10 * x + 5 && _x <= x * 30 + 10 * x + 35 && _y >= y * 30 + 10 * y + 5 && _y <= y * 30 + 10 * y + 35))
          {
            if (VOLTAGE_BUTTON.IS_PRESSED)
            {
              VOLTAGE_BUTTON.IS_PRESSED = false;
              memcpy(SAVE.VOLTAGE_COLOR, COLOR[i], 3);
            }
            if (AMP_BUTTON.IS_PRESSED)
            {
              AMP_BUTTON.IS_PRESSED = false;
              memcpy(SAVE.AMP_COLOR, COLOR[i], 3);
            } 
            if (WATTS_BUTTON.IS_PRESSED)
            {
              WATTS_BUTTON.IS_PRESSED = false;
              memcpy(SAVE.WATTS_COLOR, COLOR[i], 3);
            }
            if (GRID_GRAPHICS_BUTTON.IS_PRESSED)
            {
              GRID_GRAPHICS_BUTTON.IS_PRESSED = false;
              memcpy(SAVE.GRID_GRAPHICS_COLOR, COLOR[i], 3);
            } 
            EEPROM_SAVE();                                          //При нажатии на клетку с цветом, сохраняем цвет в EEPROM и переключаемся на страцицу настроек
            DATA.DISPLAY_NUMBER = 2;
            DATA.IS_SIZE        = true;
            DISPLAY_STATIC();
            return;
          }
          ++i;
        }
  
}

#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif
