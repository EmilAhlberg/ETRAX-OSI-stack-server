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
#include "tcp.hh"

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
  mySourceIPAddress = ipHeader->sourceIPAddress;
  myProtocol = ipHeader->protocol;
  myTOS = ipHeader->TypeOfService;
  myIdentification = ipHeader->identification;
  if (ipHeader-> destinationIPAddress == IP::instance().myAddress()) {
    //Check if the packet was intended for us.


    if (myProtocol == 1) {
      //check if ICMP
      ICMPInPacket icmpPacket (myData + headerOffset(), myLength- headerOffset(), this);
      icmpPacket.decode();
      return;
    }
    else if (myProtocol == 6) {
      TCPInPacket tcpPacket (myData + headerOffset(), myLength- headerOffset(), this, mySourceIPAddress);
      tcpPacket.decode();
      return;
    }
  }
}

void IPInPacket::answer(byte* theData, udword theLength) {
  IPHeader* ipHeader = (IPHeader*) (theData-headerOffset());

  ipHeader->versionNHeaderLength = 0x45;
  ipHeader->TypeOfService = myTOS;
  ipHeader->totalLength = HILO(theLength+headerOffset());
  ipHeader->identification = myIdentification;
  ipHeader->fragmentFlagsNOffset = HILO(0x4000);
  // "Don't fragment"-flag

  ipHeader-> timeToLive = 64;
  // Recommended ttl: 64.

  ipHeader->sourceIPAddress = IP::instance().myAddress();
  ipHeader->destinationIPAddress = mySourceIPAddress;
  //Swap dest/source address.

  ipHeader->protocol = myProtocol;

  ipHeader->headerChecksum = 0;
  ipHeader->headerChecksum = calculateChecksum(theData-headerOffset(), headerOffset(), 0);
  // chksum calculated after resetting chksum field


  myFrame->answer(theData-headerOffset(), theLength+headerOffset());
  //??? assumes that ip-header is always missing
}

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


InPacket*
IPInPacket::copyAnswerChain()
{
  IPInPacket* anAnswerPacket = new IPInPacket(*this);
  anAnswerPacket->setNewFrame(myFrame->copyAnswerChain());
  return anAnswerPacket;
}
