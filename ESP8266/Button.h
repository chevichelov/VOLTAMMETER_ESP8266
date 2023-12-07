#pragma once
#include <Arduino.h>

//Класс для работы с кнопками
class BUTTON {
   private:
    uint16_t X      = 0;
    uint16_t Y      = 0;
    uint8_t WIDTH   = 0;
    uint8_t HEIGHT  = 0;
  public:  
    bool IS_PRESSED  = false;

    //Инициализируем кнопку
    BUTTON(uint16_t X, uint16_t Y, uint8_t WIDTH, uint8_t HEIGHT)
    {
      this->X             = X;
      this->Y             = Y;
      this->HEIGHT        = HEIGHT;
      this->WIDTH         = WIDTH;
    }

    //Проверяем нажата ли кнопка
    bool IS_PRESS(uint16_t &x, uint16_t &y) 
    {
        bool _IS_PRESSED = false;
        if (x >= X && x <= X + WIDTH && y >= Y && y <= Y + HEIGHT)
        {
          if(IS_PRESSED)
            IS_PRESSED = false;
          else
            IS_PRESSED = true;
          _IS_PRESSED = true;
        }
        return _IS_PRESSED;
    }
};
