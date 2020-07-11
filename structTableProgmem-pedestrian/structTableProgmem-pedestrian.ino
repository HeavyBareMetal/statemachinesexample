#include "flashLeds.h"
#include "ioPins.h"
#include <avr/pgmspace.h>

enum class trafStates : uint8_t
{
    roadGo = 0,             //Normal road green state - wait for button
    waitRoadAmber,          //Wait road green, then turn amber, then next
    roadAmber,              //Wait amber, then red
    roadRedToCross,         //Wait red, then ped green
    pedGreen,               //Wait ped green, then flash ped green
    pedFlashGreen,          //Flash ped green, then ped red
    pedRed,                 //Wait ped red, then road amber
    roadAmberToGreen,       //Wait road amber, then road green
    roadWaitGreen           //Wait at green for traffic to clear
};

enum class PedLed : uint8_t     { red, green, greenFl };
enum class RoadLed : uint8_t    { red, green, amber };

const uint8_t _pedGreenFlashLimit_ = 20;

struct StateData
{
    PedLed pedLed;
    RoadLed roadLed;
    trafStates nextState;
    bool bStoreCrossRqst:1;
//    const bool bAdvOnCrossRqst:1;
    uint32_t nextWaitInt;
};

constexpr StateData stateDataTable[] PROGMEM =
{
    //Set these colours when advance
    {   PedLed::red,    RoadLed::green,     trafStates::waitRoadAmber,      1,  1000,   },
    {   PedLed::red,    RoadLed::amber,     trafStates::roadAmber,          0,  4000,   },
    {   PedLed::red,    RoadLed::red,       trafStates::roadRedToCross,     0,  2000,   },
    {   PedLed::red,    RoadLed::red,       trafStates::pedGreen,           0,  2000,   },
    {   PedLed::green,  RoadLed::red,       trafStates::pedFlashGreen,      0,  3000,   },
    {   PedLed::greenFl,RoadLed::red,       trafStates::pedRed,             0,  100,    },
    {   PedLed::red,    RoadLed::red,       trafStates::roadAmberToGreen,   1,  2000,   },
    {   PedLed::red,    RoadLed::amber,     trafStates::roadWaitGreen,      1,  1000,   },
    {   PedLed::red,    RoadLed::green,     trafStates::roadGo,             1,  5000,   },
};

void setupPins()
{
    pinMode(_buttonPin_, INPUT_PULLUP);
    pinMode(_pedRed_, OUTPUT);
    pinMode(_pedGreen_, OUTPUT);
    pinMode(_roadRed_, OUTPUT);
    pinMode(_roadAmber_, OUTPUT);
    pinMode(_roadGreen_, OUTPUT);
}

void setup() 
{
    setupPins();
    Serial.begin(115200);
    flashLeds();
    digitalWrite(_roadGreen_, HIGH);
    digitalWrite(_pedRed_, HIGH);
}

uint32_t lastSwitchTime = 0;
uint32_t nextSwitchInterval = 0;
uint8_t flashCount = 0;
trafStates state = trafStates::roadGo;
bool g_crossRqsted = false;

void setPedLed(PedLed ped)
{
    if (PedLed::red == ped)
    {
        digitalWrite(_pedRed_, HIGH);
        digitalWrite(_pedGreen_, LOW);
    } else if (PedLed::green == ped)
    {
        digitalWrite(_pedRed_, LOW);
        digitalWrite(_pedGreen_, HIGH);
    } else if (PedLed::greenFl == ped)
    {
        digitalWrite(_pedRed_, LOW);
        digitalWrite(_pedGreen_, flashCount++ & 0x01);
    }
}

void setRoadLed(RoadLed road)
{
    if (RoadLed::red == road)
    {
        digitalWrite(_roadGreen_, LOW);
        digitalWrite(_roadAmber_, LOW);
        digitalWrite(_roadRed_, HIGH);
    } else if (RoadLed::amber == road)
    {
        digitalWrite(_roadGreen_, LOW);
        digitalWrite(_roadAmber_, HIGH);
        digitalWrite(_roadRed_, LOW);
    } else if (RoadLed::green == road)
    {
        digitalWrite(_roadGreen_, HIGH);
        digitalWrite(_roadAmber_, LOW);
        digitalWrite(_roadRed_, LOW);
    }
}

void runStructTable(uint32_t timeMs, bool bCrossRqst)
{
    StateData ramData;
    memcpy_P(&ramData, &stateDataTable[(uint8_t)state], sizeof(StateData));
    
    
    if (ramData.bStoreCrossRqst && bCrossRqst) g_crossRqsted = true;

    bool bRunState = false;
    bool bTimedOut = ((timeMs - lastSwitchTime) >= nextSwitchInterval);
    bool bAdvNextState = false;
    if (trafStates::roadGo == state)
    {
        bRunState = g_crossRqsted && bTimedOut;
        if (bRunState) g_crossRqsted = false;
    }
    else bRunState = bTimedOut;

    if (bRunState)
    {
        setPedLed(ramData.pedLed);
        setRoadLed(ramData.roadLed);
        
        nextSwitchInterval = ramData.nextWaitInt;
        lastSwitchTime = timeMs;
        if (trafStates::pedFlashGreen == state)
        {
            if (_pedGreenFlashLimit_ <= flashCount)
            {
                flashCount = 0;
                bAdvNextState = true;
            }
        } else bAdvNextState = bTimedOut;
        if (bAdvNextState)  state = ramData.nextState;
    }
}

bool lastButtState = false;
void loop() 
{
    uint32_t timeMs = millis();
    bool nowButtState = digitalRead(_buttonPin_);
    bool bCross = false;
    if (nowButtState != lastButtState)
    {
        lastButtState = nowButtState;
        Serial.print(timeMs);
        Serial.print(" Button is now: ");
        Serial.println(nowButtState?"not pressed":"pressed");
        if (!nowButtState)  bCross = true;
    }
    runStructTable(timeMs, bCross);
}
