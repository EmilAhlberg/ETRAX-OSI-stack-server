#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}


#include "ipaddr.hh"
#include "ip.hh"
#include "icmp.hh"

IP&
IP::instance()
{
  static IP myInstance;
  return myInstance;
}

IP::IP() {
  myIPAddress = new IPAddress(130,235,200,101);
}


const IPAddress& IP::myAddress() {
  return *myIPAddress;
}

//----------------------------
//IPInPacket::
//----------------------------
IPInPacket::IPInPacket(byte* theData, udword theLength, InPacket* theFrame)
: InPacket(theData, theLength, theFrame) {
}

/*
class IPHeader
{
 public:
  IPHeader() {}

  byte  versionNHeaderLength;
  byte  TypeOfService;
  uword totalLength;
  uword identification;
  uword fragmentFlagsNOffset;
  byte  timeToLive;
  byte  protocol;
  uword headerChecksum;
  IPAddress sourceIPAddress;
  IPAddress destinationIPAddress;
};
*/

uword IPInPacket::headerOffset() { return IP::ipHeaderLength; }

void IPInPacket::decode() {
  IPHeader* ipHeader = (IPHeader*) myData;
  if (ipHeader-> destinationIPAddress == IP::instance().myAddress()) {
    //Check if the packet was intended for us.

    ipHeader-> timeToLive = 64;
    // Recommended ttl: 64.

    IPAddress tempIPSourceAddress = ipHeader->sourceIPAddress;
    // Temp var for swapping

    ipHeader->sourceIPAddress = ipHeader ->destinationIPAddress;
    ipHeader->destinationIPAddress = tempIPSourceAddress;
    //Swap dest/source address.

    ipHeader->headerChecksum = 0;
    ipHeader->headerChecksum = calculateChecksum(myData, headerOffset(), 0);
    // chksum calculated after resetting chksum field

    if (ipHeader-> protocol == 1) {
      //check if ICMP
      ICMPInPacket icmpPacket (myData + headerOffset(), myLength- headerOffset(), this);
      icmpPacket.decode();
      return;
    }
    // TODO: TCP?
  }
}

void IPInPacket::answer(byte* theData, udword theLength) {
    myFrame->answer(theData-headerOffset(), theLength+headerOffset());
    //??? assumes that ip-header is always missing
}
