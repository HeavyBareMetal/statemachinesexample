#include "flashLeds.h"
#include "ioPins.h"

enum class trafStates : uint8_t
{
    roadGo = 0,             //Normal road green state - wait for button
    waitRoadAmber,          //Wait road green, then turn amber, then next
    roadAmber,              //Wait amber, then red
    roadRedToCross,         //Wait red, then ped green
    pedGreen,               //Wait ped green, then flash ped green
    pedFlashGreen,          //Flash ped green, then ped red
    pedRed,                 //Wait ped red, then road amber
    roadAmberToGreen        //Wait road amber, then road green
};

const uint32_t _waitRoadAmberTime_ = 1000;
const uint32_t _roadAmberTime_ = 4000;
const uint32_t _roadRedToCrossTime_ = 3000;
const uint32_t _pedGreenTime_ = 2000;
const uint32_t _pedRedTime_ = 2000;
const uint8_t _pedGreenFlashLimit_ = 20;
const uint32_t _pedGreenFlashTime_ = 100;
const uint32_t _roadAmberToGreenTime_ = 5000;

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

void runSwitchCase(uint32_t timeMs, bool bCrossRqst)
{
//    trafStates newState = state;

    switch (state)
    {
        case trafStates::roadGo:
        {   //If cross requested, go to next
            g_crossRqsted |= bCrossRqst;
            if (g_crossRqsted && 
                ((timeMs - lastSwitchTime) >= nextSwitchInterval))
            {
                g_crossRqsted = false;
                state = trafStates::waitRoadAmber;
                nextSwitchInterval = _waitRoadAmberTime_;
                lastSwitchTime = timeMs;
            }
            break;
        }
        case trafStates::waitRoadAmber:
        {
            if ((timeMs - lastSwitchTime) >= nextSwitchInterval)
            {
                digitalWrite(_roadGreen_, LOW);
                digitalWrite(_roadAmber_, HIGH);
                nextSwitchInterval = _roadAmberTime_;
                state = trafStates::roadAmber;
                lastSwitchTime = timeMs;
            }
            break;
        }
        case trafStates::roadAmber:
        {
            if ((timeMs - lastSwitchTime) >= nextSwitchInterval)
            {
                digitalWrite(_roadAmber_, LOW);
                digitalWrite(_roadRed_, HIGH);
                nextSwitchInterval = _pedGreenTime_;
                state = trafStates::roadRedToCross;
                lastSwitchTime = timeMs;
            }
            break;
        }
        case trafStates::roadRedToCross:
        {
            if ((timeMs - lastSwitchTime) >= nextSwitchInterval)
            {
                digitalWrite(_pedRed_, LOW);
                digitalWrite(_pedGreen_, HIGH);
                nextSwitchInterval = _pedGreenTime_;
                state = trafStates::pedGreen;
                lastSwitchTime = timeMs;
            }
            break;
        }
        case trafStates::pedGreen:
        {
            if ((timeMs - lastSwitchTime) >= nextSwitchInterval)
            {
                nextSwitchInterval = _pedGreenFlashTime_;
                state = trafStates::pedFlashGreen;
                lastSwitchTime = timeMs;
                flashCount = 0;
            }
            g_crossRqsted |= bCrossRqst;
            break;
        }
        case trafStates::pedFlashGreen:
        {
            if ((timeMs - lastSwitchTime) >= nextSwitchInterval)
            {
                digitalWrite(_pedGreen_, flashCount++ & 0x01);
                nextSwitchInterval = _pedGreenFlashTime_;
                lastSwitchTime = timeMs;
                if (_pedGreenFlashLimit_ <= flashCount)
                {
                    state = trafStates::pedRed;
                    digitalWrite(_pedGreen_, LOW);
                    digitalWrite(_pedRed_, HIGH);
                    nextSwitchInterval = _pedRedTime_;
                    lastSwitchTime = timeMs;
                }
            }
            g_crossRqsted |= bCrossRqst;
            break;
        }
        case trafStates::pedRed:
        {
            if ((timeMs - lastSwitchTime) >= nextSwitchInterval)
            {
                digitalWrite(_roadAmber_, HIGH);
                digitalWrite(_roadRed_, LOW);
                nextSwitchInterval = _waitRoadAmberTime_;
                state = trafStates::roadAmberToGreen;
                lastSwitchTime = timeMs;
            }
            g_crossRqsted |= bCrossRqst;
            break;
        }
        case trafStates::roadAmberToGreen:
        {
            g_crossRqsted |= bCrossRqst;
            if ((timeMs - lastSwitchTime) >= nextSwitchInterval)
            {
                digitalWrite(_roadAmber_, LOW);
                digitalWrite(_roadGreen_, HIGH);
                state = trafStates::roadGo;
                nextSwitchInterval = _roadAmberToGreenTime_;
                lastSwitchTime = timeMs;
            }
            break;
        }
        default: state = trafStates::roadGo;
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
    runSwitchCase(timeMs, bCross);
}
