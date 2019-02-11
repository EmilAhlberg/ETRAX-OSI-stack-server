/*!***************************************************************************
*!
*! FILE NAME  : tcp.cc
*!
*! DESCRIPTION: TCP, Transport control protocol
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
#include "timr.h"
}

#include "iostream.hh"
#include "tcp.hh"
#include "ip.hh"
#include "ethernet.hh"

#define D_TCP
#ifdef D_TCP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** TCP DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
TCP::TCP()
{
  trace << "TCP created." << endl;
}

//----------------------------------------------------------------------------
//
TCP&
TCP::instance()
{
  static TCP myInstance;
  return myInstance;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::getConnection(IPAddress& theSourceAddress,
                   uword      theSourcePort,
                   uword      theDestinationPort)
{
  TCPConnection* aConnection = NULL;
  // Find among open connections
  uword queueLength = myConnectionList.Length();
  myConnectionList.ResetIterator();
  bool connectionFound = false;
  while ((queueLength-- > 0) && !connectionFound)
  {
    aConnection = myConnectionList.Next();
    connectionFound = aConnection->tryConnection(theSourceAddress,
                                                 theSourcePort,
                                                 theDestinationPort);
  }
  if (!connectionFound)
  {
    trace << "Connection not found!" << endl;
    aConnection = NULL;
  }
  else
  {
    trace << "Found connection in queue" << endl;
  }
  return aConnection;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::createConnection(IPAddress& theSourceAddress,
                      uword      theSourcePort,
                      uword      theDestinationPort,
                      InPacket*  theCreator)
{
  TCPConnection* aConnection =  new TCPConnection(theSourceAddress,
                                                  theSourcePort,
                                                  theDestinationPort,
                                                  theCreator);
  myConnectionList.Append(aConnection);
  return aConnection;
}

//----------------------------------------------------------------------------
//
void
TCP::deleteConnection(TCPConnection* theConnection)
{
  myConnectionList.Remove(theConnection);
  delete theConnection;
}

//----------------------------------------------------------------------------
//
TCPConnection::TCPConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort,
                             InPacket*  theCreator):
        hisAddress(theSourceAddress),
        hisPort(theSourcePort),
        myPort(theDestinationPort),
        sendNext(222001)
{
  trace << "TCP connection created" << endl;
  myTCPSender = new TCPSender(this, theCreator),
  myState = ListenState::instance();
}

//----------------------------------------------------------------------------
//
TCPConnection::~TCPConnection()
{
  trace << "TCP connection destroyed" << endl;
  delete myTCPSender;
}

//----------------------------------------------------------------------------
//
bool
TCPConnection::tryConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort)
{
  return (theSourcePort      == hisPort   ) &&
         (theDestinationPort == myPort    ) &&
         (theSourceAddress   == hisAddress);
}


// TCPConnection cont...

void TCPConnection::Synchronize(udword theSynchronizationNumber) {
  myState->Synchronize(this, theSynchronizationNumber);
}


//----------------------------------------------------------------------------
// TCPState contains dummies for all the operations, only the interesting ones
// gets overloaded by the various sub classes.


//----------------------------------------------------------------------------
//
void
TCPState::Kill(TCPConnection* theConnection)
{
  trace << "TCPState::Kill" << endl;
  TCP::instance().deleteConnection(theConnection);
}







//----------------------------------------------------------------------------
//
ListenState*
ListenState::instance()
{
  static ListenState myInstance;
  return &myInstance;
}

void ListenState::Synchronize(TCPConnection* theConnection, udword theSynchronizationNumber) {
  switch (theConnection->myPort)
  {
   case 7:
     trace << "got SYN on ECHO port" << endl;
     theConnection->receiveNext = theSynchronizationNumber + 1;
     theConnection->receiveWindow = 8*1024;
     theConnection->sendNext = get_time();
     // Next reply to be sent.
     theConnection->sentUnAcked = theConnection->sendNext;
     // Send a segment with the SYN and ACK flags set.
     theConnection->myTCPSender->sendFlags(0x12);
     // Prepare for the next send operation.
     theConnection->sendNext += 1;
     // Change state
     theConnection->myState = SynRecvdState::instance();
     break;
   default:
     trace << "send RST..." << endl;
     theConnection->sendNext = 0;
     // Send a segment with the RST flag set.
     theConnection->myTCPSender->sendFlags(0x04);
     TCP::instance().deleteConnection(theConnection);
     break;
  }
}




//----------------------------------------------------------------------------
//
SynRecvdState*
SynRecvdState::instance()
{
  static SynRecvdState myInstance;
  return &myInstance;
}


void SynRecvdState::Acknowledge(TCPConnection* theConnection,
                 udword theAcknowledgementNumber) {}




//----------------------------------------------------------------------------
//
EstablishedState*
EstablishedState::instance()
{
  static EstablishedState myInstance;
  return &myInstance;
}


//----------------------------------------------------------------------------
//
void
EstablishedState::NetClose(TCPConnection* theConnection)
{
  trace << "EstablishedState::NetClose" << endl;

  // Update connection variables and send an ACK

  // Go to NetClose wait state, inform application
  theConnection->myState = CloseWaitState::instance();

  // Normally the application would be notified next and nothing
  // happen until the application calls appClose on the connection.
  // Since we don't have an application we simply call appClose here instead.

  // Simulate application Close...
  theConnection->AppClose();
}

void TCPConnection::AppClose() {

}

void TCPConnection::Kill() {

}

//----------------------------------------------------------------------------
//
void
EstablishedState::Receive(TCPConnection* theConnection,
                          udword theSynchronizationNumber,
                          byte*  theData,
                          udword theLength)
{
  trace << "EstablishedState::Receive" << endl;

  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment, then echo the data using Send.



}

void EstablishedState::Acknowledge(TCPConnection* theConnection,
                        udword theAcknowledgementNumber) {}


void EstablishedState::Send(TCPConnection* theConnection,
                 byte*  theData,
                 udword theLength) {}




//----------------------------------------------------------------------------
//
CloseWaitState*
CloseWaitState::instance()
{
  static CloseWaitState myInstance;
  return &myInstance;
}




void CloseWaitState::AppClose(TCPConnection* theConnection) {

}





//----------------------------------------------------------------------------
//
LastAckState*
LastAckState::instance()
{
  static LastAckState myInstance;
  return &myInstance;
}



void LastAckState::Acknowledge(TCPConnection* theConnection,
                 udword theAcknowledgementNumber){}


void TCPState::Synchronize(TCPConnection* theConnection,
                        udword theSynchronizationNumber) {}
// Handle an incoming SYN segment
void TCPState::NetClose(TCPConnection* theConnection) {}
// Handle an incoming FIN segment
void TCPState::AppClose(TCPConnection* theConnection) {}
// Handle close from application
void TCPState::Receive(TCPConnection* theConnection,
                    udword theSynchronizationNumber,
                    byte*  theData,
                    udword theLength) {}
// Handle incoming data
void TCPState::Acknowledge(TCPConnection* theConnection,
                        udword theAcknowledgementNumber) {}
// Handle incoming Acknowledgement
void TCPState::Send(TCPConnection* theConnection,
                 byte*  theData,
                 udword theLength) {}


//----------------------------------------------------------------------------
//
TCPSender::TCPSender(TCPConnection* theConnection,
                     InPacket*      theCreator):
        myConnection(theConnection),
        myAnswerChain(theCreator->copyAnswerChain()) // Copies InPacket chain!
{
}

//----------------------------------------------------------------------------
//
TCPSender::~TCPSender()
{
  myAnswerChain->deleteAnswerChain();
}

void
TCPSender::sendFlags(byte theFlags)
{
  int totalDataLength = Ethernet::ethernetHeaderLength + IP::ipHeaderLength + TCP::tcpHeaderLength;
  int hoffs = Ethernet::ethernetHeaderLength + IP::ipHeaderLength;
  int totalSegmentLength = TCP::tcpHeaderLength;
  // Decide on the value of the length totalSegmentLength.
  // Allocate a TCP segment.
  byte* anAnswer = new byte[totalDataLength];

  // Calculate the pseudo header checksum
  TCPPseudoHeader* aPseudoHeader =
    new TCPPseudoHeader(myConnection->hisAddress,
                        totalSegmentLength);
  uword pseudosum = aPseudoHeader->checksum();
  delete aPseudoHeader;
  // Create the TCP segment.
  TCPHeader* tcpHeader = (TCPHeader*) anAnswer+hoffs;
  tcpHeader->sourcePort = HILO(myConnection->myPort);
  tcpHeader->destinationPort = HILO(myConnection->hisPort);
  tcpHeader->sequenceNumber = LHILO(myConnection->sendNext);
  tcpHeader->acknowledgementNumber = LHILO(myConnection->receiveNext);
  tcpHeader->headerLength = (TCP::tcpHeaderLength/4) * 16;
  // since it only occupies 4 bits
  tcpHeader->flags = theFlags;
  tcpHeader->windowSize = HILO(myConnection->receiveWindow);
  tcpHeader->checksum = 0;
  // Calculate the final checksum.
  tcpHeader->checksum = calculateChecksum((byte*)tcpHeader,
                                           totalSegmentLength,
                                           pseudosum);
  // Send the TCP segment.
  myAnswerChain->answer((byte*)tcpHeader,
                        totalSegmentLength);
  // Deallocate the dynamic memory
  // delete anAnswer;
}

//----------------------------------------------------------------------------
//
TCPInPacket::TCPInPacket(byte*           theData,
                         udword          theLength,
                         InPacket*       theFrame,
                         IPAddress&      theSourceAddress):
        InPacket(theData, theLength, theFrame),
        mySourceAddress(theSourceAddress)
{
}

void TCPInPacket::decode() {
  TCPHeader* tcpHeader = (TCPHeader*) myData;
  mySourcePort = HILO(tcpHeader->sourcePort);
  myDestinationPort = HILO(tcpHeader->destinationPort);
  mySequenceNumber = LHILO(tcpHeader->sequenceNumber);
  myAcknowledgementNumber = LHILO(tcpHeader->acknowledgementNumber);
  TCPConnection* tcpConnection = TCP::instance().getConnection(mySourceAddress, mySourcePort, myDestinationPort);
  if (tcpConnection == NULL) {
    tcpConnection = TCP::instance().createConnection(mySourceAddress, mySourcePort, myDestinationPort, this);

    if ((tcpHeader->flags & 0x02) != 0)
      {
        // State LISTEN. Received a SYN flag.
        tcpConnection->Synchronize(mySequenceNumber);
        //////////////////////
        tcpConnection->myState = SynRecvdState::instance();
        /////////////////////
      }
      else
      {
        // State LISTEN. No SYN flag. Impossible to continue.
        tcpConnection->Kill();
      }
    }
  else
  {
    // Connection was established. Handle all states.

      //CHECK CONDITIONS
      //in SynRecvdState:
      if ("synACKrecived") {
        // ftp://nic.funet.fi/pub/networking/documents/rfc/rfc793.txt page 23
        tcpConnection->myState = EstablishedState::instance();
      } else if (//"CLOSE") {
        //send FIN WAIT-1
        tcpConnection->myState = CloseWaitState::instance(); ??
      }
      //in EstablishedState:
      if("CLOSE") {
        //send FIN WAIT-1
        tcpConnection->myState = CloseWaitState::instance(); ??
      }

      //in CloseWaitState 1
      if ("Recieves FIN") {
        //send FIN ACK
        //closing ??
      }
      else if ("Recieves ACK of FIN") {
        //FINWAIT-2
      }

      //in CloseWaitState 2
      if ("Recieves FIN") {
        //send FIN ACK
        //time wait
      }


      //HANDLE STATES

  }
}




void TCPInPacket::answer(byte* theData, udword theLength) {}
uword TCPInPacket::headerOffset() { return 0; }

//----------------------------------------------------------------------------
//
InPacket*
TCPInPacket::copyAnswerChain()
{
  return myFrame->copyAnswerChain();
}

//----------------------------------------------------------------------------
//
TCPPseudoHeader::TCPPseudoHeader(IPAddress& theDestination,
                                 uword theLength):
        sourceIPAddress(IP::instance().myAddress()),
        destinationIPAddress(theDestination),
        zero(0),
        protocol(6)
{
  tcpLength = HILO(theLength);
}

//----------------------------------------------------------------------------
//
uword
TCPPseudoHeader::checksum()
{
  return calculateChecksum((byte*)this, 12);
}


/****************** END OF FILE tcp.cc *************************************/
