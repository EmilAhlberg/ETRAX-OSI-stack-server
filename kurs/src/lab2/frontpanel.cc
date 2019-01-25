/*!***************************************************************************
*!
*! FILE NAME  : FrontPanel.cc
*!
*! DESCRIPTION: Handles the LED:s
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
/**
#include "sp_alloc.h"
#include "system.h"
#include "osys.h"
*/
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
byte LED::writeOutRegisterShadow = 0x38;

LED::LED(byte theLedNumber) {
  myLedBit = theLedNumber;
  iAmOn = false;
}
// Constructor: initiate the bitmask 'myLedBit'.

void LED::on() {
    *(VOLATILE byte*)0x80000000 = LED::writeOutRegisterShadow &= ~myLedBit;
}
// turn this led on
void LED::off() {
    *(VOLATILE byte*)0x80000000 = LED::writeOutRegisterShadow |= myLedBit;
}
// turn this led off
void LED::toggle() {
    *(VOLATILE byte*)0x80000000 = LED::writeOutRegisterShadow ^= myLedBit;
}
// toggle this led

NetworkLEDTimer::NetworkLEDTimer(Duration blinkTime) {
    myBlinkTime = blinkTime;
}

void NetworkLEDTimer::start() {
    this->timeOutAfter(myBlinkTime);
}

void NetworkLEDTimer::timeOut() {
    FrontPanel::instance().notifyLedEvent(FrontPanel::instance().networkLedId);
}



CDLEDTimer::CDLEDTimer(Duration blinkPeriod) {
    this->timerInterval(blinkPeriod);
    this->startPeriodicTimer();
}

void CDLEDTimer::timerNotify() {
    FrontPanel::instance().notifyLedEvent(FrontPanel::instance().cdLedId);
}


StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod) {
    this->timerInterval(blinkPeriod);
    this->startPeriodicTimer();
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

FrontPanel::FrontPanel() :
  Job(),
  myNetworkLED(LED::LED(networkLedId)),
  myCDLED(LED::LED(cdLedId)),
  myStatusLED(LED::LED(statusLedId))
{
    mySemaphore = Semaphore::createQueueSemaphore("name", 0);
    netLedEvent = false;
    cdLedEvent = false;
    statusLedEvent = false;
    Job::schedule(this);
}

void
FrontPanel::doit()
{
    myNetworkLEDTimer = new NetworkLEDTimer(10);
    myCDLEDTimer = new CDLEDTimer(100);
    myStatusLEDTimer = new StatusLEDTimer(20);

    while(true)
    {
        mySemaphore->wait();
      if(netLedEvent) {
            myNetworkLED.off();
            netLedEvent = false;
        }
         if(cdLedEvent) {
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
    cout << "Packet received." << endl;
    myNetworkLED.on();
    //ax_printf("\r\n\r\nPacket received.\r\n");
    myNetworkLEDTimer->start();
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
