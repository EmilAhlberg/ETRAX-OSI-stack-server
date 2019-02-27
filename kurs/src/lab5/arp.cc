#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}
#include "iostream.hh"

#include "arp.hh"
#include "ip.hh"
#include "ethernet.hh"


ARPInPacket::ARPInPacket(byte* theData, udword theLength, InPacket* theFrame)
: InPacket(theData, theLength, theFrame) {
}

uword ARPInPacket::headerOffset() { return 0; }
  //??? Assumed not to be needed

void ARPInPacket::decode() {
  ARPHeader* arpHeader = (ARPHeader*) myData;
  if (arpHeader-> targetIPAddress == IP::instance().myAddress()) {
    //Check if we should reply.
    arpHeader-> op = 2 << 8;
    //Set header to reply.

    EthernetAddress  tempEthSenderAddress = arpHeader-> senderEthAddress;
    IPAddress tempIPSenderAddress = arpHeader-> senderIPAddress;
    // Temp vars for swapping

    arpHeader-> senderEthAddress = Ethernet::instance().myAddress();
    // Set senderEthAddress to our MAC address (no swap since broadcast)
    arpHeader-> senderIPAddress = arpHeader -> targetIPAddress;
    arpHeader-> targetEthAddress = tempEthSenderAddress;
    arpHeader-> targetIPAddress = tempIPSenderAddress;
    // Swap IP and MAC for the rest

    this->answer(myData, myLength);
  }
}

void ARPInPacket::answer(byte* theData, udword theLength) {
    myFrame->answer(theData, theLength);
}
