/*!***************************************************************************
*!
*! FILE NAME  : llc.cc
*!
*! DESCRIPTION: LLC dummy
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}

#include "iostream.hh"
#include "ethernet.hh"
#include "llc.hh"
#include "arp.hh"
#include "ip.hh"

//#define D_LLC
#ifdef D_LLC
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** LLC DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
LLCInPacket::LLCInPacket(byte*           theData,
                         udword          theLength,
						 InPacket*       theFrame,
                         EthernetAddress theDestinationAddress,
                         EthernetAddress theSourceAddress,
                         uword           theTypeLen):
InPacket(theData, theLength, theFrame),
myDestinationAddress(theDestinationAddress),
mySourceAddress(theSourceAddress),
myTypeLen(theTypeLen)
{
}

//----------------------------------------------------------------------------
//
void
LLCInPacket::decode()
{
  trace << "to " << myDestinationAddress << " from " << mySourceAddress
   << " typeLen " << myTypeLen << endl;

   if (myTypeLen == 0x806 && myDestinationAddress == Ethernet::instance().broadcastAddress()) {
     // Check if ARP and broadcast ??? Maybe change to handle ARP replies to us?
     ARPInPacket arpPacket (myData, myLength, this);
     arpPacket.decode();
     return;
   }

  if (myDestinationAddress == Ethernet::instance().myAddress() && myTypeLen == 0x800) {
    // Check if IP packet and sent to us
    IPInPacket ipPacket (myData, myLength, this);
    ipPacket.decode();
    return;
  }
}

//----------------------------------------------------------------------------
//
void
LLCInPacket::answer(byte *theData, udword theLength)
{
  myFrame->answer(theData, theLength);
}

uword
LLCInPacket::headerOffset()
{
  return myFrame->headerOffset() + 0;
}

/****************** END OF FILE Ethernet.cc *************************************/
