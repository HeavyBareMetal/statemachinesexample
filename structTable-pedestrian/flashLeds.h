#ifndef _FLASH_LEDS_H_
#define _FLASH_LEDS_H_

#include "ioPins.h"

void flashLeds()
{
    digitalWrite(_pedRed_, HIGH);
    delay(200);
    digitalWrite(_pedRed_, LOW);
    delay(200);
    digitalWrite(_pedGreen_, HIGH);
    delay(200);
    digitalWrite(_pedGreen_, LOW);
    delay(200);
    digitalWrite(_roadRed_, HIGH);
    delay(200);
    digitalWrite(_roadRed_, LOW);
    delay(200);
    digitalWrite(_roadAmber_, HIGH);
    delay(200);
    digitalWrite(_roadAmber_, LOW);
    delay(200);
    digitalWrite(_roadGreen_, HIGH);
    delay(200);
    digitalWrite(_roadGreen_, LOW);
    delay(200);
}

#endif //_FLASH_LEDS_H_
