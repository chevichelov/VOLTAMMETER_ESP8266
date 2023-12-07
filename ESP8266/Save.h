#pragma once
#include <Arduino.h>

//Класс для сохранения данных в EEPROM
class SAVE {
  public:
    //Данные калибровки тачпад
    uint16_t CAL_DATA[5];
    //Режим отображения графики
    uint8_t GRAPH_OPTION            = 0;
    //Записывать или нет данные в лог файл
    bool SD_WRITE                   = true;

    //Цвет напряжения
    uint8_t VOLTAGE_COLOR[3]        = {255, 255, 255};
    //Цвет силы тока
    uint8_t AMP_COLOR[3]            = {255, 0, 0};
    //Цвет ватт
    uint8_t WATTS_COLOR[3]          = {0, 255, 0};
    //Цвет графика
    uint8_t GRID_GRAPHICS_COLOR[3]  = {247, 53, 255};

    //Требуется ли калибровка тачпада 0 - нет / 5 да
    uint8_t IS_TOUCH                = 0;

    //Логин от WI-FI
    char SSID[50]                   = "логин";
    //пароль от WI-FI
    char PASSWORD[50]               = "пароль";

    //Количество попыток подключения
    uint16_t CONNECTED_COUNT        = 1000;

    //MAC адрес
    uint8_t MAC[6]                  = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
    //IP адрес
    uint8_t IP[4]                   = {192, 168, 0, 35};
    //IP шлюза
    uint8_t GATEWAY[4]              = {192, 168, 0, 1};
    //Маска сети
    uint8_t SUBNET[4]               = {255, 255, 255, 0};
    //ДНС
    uint8_t DNS[4]                  = {109, 195 ,80, 4};
    //Порт
    uint16_t PORT                   = 80;

    //WI-FI включён или нет
    bool IS_WIFI                    = true;
};
