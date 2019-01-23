/*!***************************************************************************
*!
*! FILE NAME  : FrontPanel.cc
*!
*! DESCRIPTION: Handles the LED:s
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "iostream.hh"
#include "frontpanel.hh"

//#define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** FrontPanel DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//

LED::LED(byte theLedNumber) {
  myLedBit = theLedNumber;
  iAmOn = false;
  writeOutRegisterShadow = 0x38;
}
// Constructor: initiate the bitmask 'myLedBit'.

void LED::on() {
    *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow &= ~led;
}
// turn this led on
void LED::off() {
    *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow |= led;
}
// turn this led off
void LED::toggle() {
    *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow ^= led;
}
// toggle this led

NetworkLEDTimer::NetworkLEDTimer(Duration blinkTime) {
    myBlinkTime = blinkTime;
}

void NetworkLEDTimer::start() {
    this->timeOutAfter(myBlinkTime);
}

void NetworkLEDTimer::timeOut() {
    FrontPanel::instance().notifyLedEvent(FrontPanel::instance().networkLedId));
}



CDLEDTimer::CDLEDTimer(Duration blinkPeriod) {
    startPeriodicTimer(blinkPeriod);
}

void CDLEDTimer::timerNotify() {
    FrontPanel::instance().notifyLedEvent(FrontPanel::instance().cdLedId);
}


StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod) {
    startPeriodicTimer(blinkPeriod);
}

void StatusLEDTimer::timerNotify() {
    FrontPanel::instance().notifyLedEvent(FrontPanel::instance().statusLedId);
}



//Singleton creation
FrontPanel&
FrontPanel::instance()
{
  static FrontPanel myInstance;
  return myInstance;
}

FrontPanel::FrontPanel()
{
    mySemaphore = Semaphore::createQueueSemaphore("name", 0);
    myNetworkLED = new LED(networkLedId);
    netLedEvent = false;
    myCDLED = new LED(cdLedId);
    cdLedEvent = false;
    myStatusLED = new LED(statusLedId);
    statusLedEvent = false;
}

void
FrontPanel::doit()
{
    myNetworkLEDTimer = new NetworkLEDTimer(10);
    myCDLEDTimer = new CDLEDTimer(10);
    myStatusLEDTimer = new StatusLEDTimer(10);

    while(true)
    {
        mySemaphore->wait();
        if(netLedEvent) {
            myNetworkLED.off();
            netLedEvent = false;
        }
        else if(cdLedEvent) {
            myCDLED.toggle();
            cdLedEvent = false;
        }
        else if(statusLedEvent) {
            myStatusLED.toggle();
            statusLedEvent = false;
        }
    }
}

void
FrontPanel::packetReceived()
{
    myNetworkLED.on();
    myNetworkLEDTimer.start();
}

void
FrontPanel::notifyLedEvent(uword theLedId)
{
    switch (theLedId) {
        case networkLedId: netLedEvent = true; break;
        case cdLedId: cdLedEvent = true; break;
        case statusLedId: statusLedEvent = true; break;
    }
    mySemaphore->signal();
}


/****************** END OF FILE FrontPanel.cc ********************************/
