#pragma once
#include <Arduino.h>

//Класс конвертации данных
class TO {
  public:

    //Функция аналог функции MAP, но в отличии от оригинала умеет работать с типом данных FLOAT
    byte MAPFLOAT(float VALUE, byte MININ, byte MAXIN, byte MINOUT, byte MAXOUT) {
      return (float)(VALUE - MININ) * (MAXOUT - MINOUT) / (float)(MAXIN - MININ) + MINOUT;
    }

    //Функция затирает предыдущие показания пробелами 
    char* DISPLAY_TEXT(float DATA, uint8_t COUNT, uint8_t FLOAT)          
    {
      static char DATA_RESULT[20];                                        //Объявляем переменную
      char DATA_DISPLAY[20];                                              //Объявляем переменную
      dtostrf(DATA, COUNT, FLOAT, DATA_DISPLAY);                          //Конвертируем показания в привычные глазу данные для дальнейшего вывода на экран
    
      uint8_t LEN = COUNT - (strlen(DATA_DISPLAY) - 1);                   //Узнаём длину полученных данных
      
      for(uint8_t i = 0; i < LEN; ++i)                                    //Вычисляем сколько пробелов не хватает
        strcpy(DATA_RESULT, " ");                                         //Создаём строку из недостающих пробелов
      
      strcat(DATA_RESULT, DATA_DISPLAY);                                  //Добавляем недостающие пробелы
      
      return DATA_RESULT;                                                 //Отдаём результат
    }

    //Функция выводит дату на экран
    String DISPLAY_DATE(uint8_t DATA)                                     
    {
      String DATE_RESULT = String(DATA);                                  //Конвертируем число в строку
      if (DATE_RESULT.length() == 1)                                      //Если число меньше 10, добавляем 0 перед числом
        DATE_RESULT = "0" + DATE_RESULT;
      return DATE_RESULT;                                                 //Выводим результат
    }

    //Функция конвертирует цвет из RGB в RGB16
    uint16_t RGB_TO_RGB16(uint8_t R, uint8_t G, uint8_t B)                
    {
      return ((R & 0xF8)<<8) | ((G & 0xFC)<<3) | ((B & 0xF8)>>3);
    }

    //Функция - аналог функции сплит
    uint8_t SPLIT(String TEXT, uint8_t POSITION)                          
    {
      uint16_t COUNT    = TEXT.length();
      uint8_t x         = 0, y = 0;
      for (uint8_t i=0; i < COUNT; i++)
        if(TEXT.charAt(i) == ',' || i == COUNT - 1)
        { 
          if (POSITION == y)
            return strtoul(TEXT.substring(x, i == COUNT - 1 ? i + 1 : i).c_str(), NULL, 0);
          ++y;
          x = i + 1;
        }
      return 0;
    }
};
