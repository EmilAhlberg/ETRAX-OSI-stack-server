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


//----------------------------
//ICMPInPacket::
//----------------------------
ICMPInPacket::ICMPInPacket(byte* theData, udword theLength, InPacket* theFrame)
: InPacket(theData, theLength, theFrame) {
}

uword ICMPInPacket::headerOffset() { return icmpHeaderLen; }

void ICMPInPacket::decode() {
  ICMPHeader* icmpHeader = (ICMPHeader*) myData;
  if (icmpHeader->type == 8 ) {
    //Echo request type.
    icmpHeader->type = 0;
    //Echo reply type.

    icmpHeader->checksum = 0;
    icmpHeader->checksum = calculateChecksum(myData, myLength, 0);
    // chksum calculated after resetting chksum field

    // ICMPECHOHeader* icmpEchoHeader = (ICMPECHOHeader*) (myData+headerOffset());

    this->answer(myData, myLength);
  }
}

void ICMPInPacket::answer(byte* theData, udword theLength) {
    myFrame->answer(theData, theLength);
}
