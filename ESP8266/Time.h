#pragma once
#include <Arduino.h>
//Класс времени
class TIME {
  public:
    //Время для расчёта емкости
    uint32_t SET_MILLISECOND        = 0;
    //Время обновленения пакетов по UART
    uint32_t TIME_DATA              = 0;
    //Время задержки нажатия кнопки
    uint32_t TIME_BUTTOM            = 0;

    //Отсчитывает секунды
    uint16_t SECOND_GRAPH           = 0;
    //Определыяет длину графика
    uint16_t MINUT_GRAPH            = 0;
    //Время обновление данных графика
    uint32_t TIME_GRAPH             = 0;

    //Определыяет длину графика мини
    uint16_t TEAK                   = 0;
    //Время обновление данных графика мини
    uint32_t TIME_GRAPH_FAST        = 0;
    
    //Время обновления данных для SD
    uint32_t TIME_LOG               = 0;
    //Отсчитывает секунды
    uint8_t SECOND_LOG              = 0;
};
