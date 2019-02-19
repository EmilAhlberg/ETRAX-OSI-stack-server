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
#include "tcpsocket.hh"

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
// LAB 5!
bool TCP::acceptConnection(uword portNo) {
  return portNo == 7;
}

void
TCP::connectionEstablished(TCPConnection *theConnection)
{
  if (theConnection->serverPortNumber() == 7)
  {
    TCPSocket* aSocket = new TCPSocket(theConnection);
    // Create a new TCPSocket.
    theConnection->registerSocket(aSocket);
    // Register the socket in the TCPConnection.
    Job::schedule(new SimpleApplication(aSocket));
    // Create and start an application for the connection.
  }
}

// Create a new TCPSocket. Register it in TCPConnection.
// Create and start a SimpleApplication.



//----------------------------------------------------------------------------
//
TCPConnection::TCPConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort,
                             InPacket*  theCreator):
        hisAddress(theSourceAddress),
        hisPort(theSourcePort),
        myPort(theDestinationPort)
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

void TCPConnection::Acknowledge(udword theAcknowledgementNumber ) {
  myState->Acknowledge(this, theAcknowledgementNumber);
}

void TCPConnection::NetClose(udword theSynchronizationNumber) {
  myState->NetClose(this, theSynchronizationNumber);
}

void TCPConnection::AppClose() {
    myState->AppClose(this);
}

void TCPConnection::Kill() {
    myState->Kill(this);
}

void TCPConnection::Receive(udword theSynchronizationNumber,byte*  theData, udword theLength) {
   myState->Receive(this, theSynchronizationNumber, theData, theLength);
}

void TCPConnection::Send(byte*  theData, udword theLength) {
    myState->Send(this, theData, theLength);
}

//----------------------------------------------------------------------------
// LAB 5!

uword TCPConnection::serverPortNumber() {
  return myPort;
}
// Return myPort.

void  TCPConnection::registerSocket(TCPSocket* theSocket) {
  mySocket = theSocket;
}
  // Set mySocket to theSocket.

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
  /*switch (theConnection->myPort)
  {
   case 7:*/
    if(TCP::instance().acceptConnection(theConnection->myPort)) {
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
       //break;
     }
   //default:
   else {
       trace << "send RST..." << endl;
       theConnection->sendNext = 0;
       // Send a segment with the RST flag set.
       theConnection->myTCPSender->sendFlags(0x04);
       TCP::instance().deleteConnection(theConnection);
       //break;
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


void SynRecvdState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
  if (theConnection->sendNext == theAcknowledgementNumber) {
    theConnection->sentUnAcked = theAcknowledgementNumber;
    theConnection->myState =  EstablishedState::instance();
    TCP::instance().connectionEstablished(theConnection);
    //Create socket for theConnection
    cout << "Conn established" << endl;
  }
}




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
EstablishedState::NetClose(TCPConnection* theConnection, udword theSynchronizationNumber)
{
  trace << "EstablishedState::NetClose" << endl;


  // Update connection variables and send an ACK
  theConnection -> receiveNext = theSynchronizationNumber +1;
  // ??? sequence check is bad?
  theConnection->myTCPSender->sendFlags(0x10);

  // Prepare for the next send operation.

  // Go to NetClose wait state, inform application
  theConnection->myState = CloseWaitState::instance();

  // Normally the application would be notified next and nothing
  // happen until the application calls appClose on the connection.
  // Since we don't have an application we simply call appClose here instead.

  // Simulate application Close...

  theConnection->mySocket->socketEof();

  //theConnection->AppClose();
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

  if (theSynchronizationNumber == theConnection->receiveNext) {
    theConnection->receiveNext = theSynchronizationNumber + theLength;
    theConnection->myTCPSender->sendFlags(0x10);
  }

  if (theConnection->myPort == 7 ) {
    theConnection->mySocket->socketDataReceived(theData, theLength);
    // theConnection->Send(theData, theLength); //LAB 4 deprecated!
  }

  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment, then echo the data using Send.
}

void EstablishedState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
    if (theConnection->sendNext == theAcknowledgementNumber) {
        theConnection->sentUnAcked = theAcknowledgementNumber;
        if(!theConnection->myTCPSender->sendFromQueue()) {
            // no more data in queue
            theConnection->mySocket->socketDataSent();
            //release write semaphore
        }
    }
}


void EstablishedState::Send(TCPConnection* theConnection, byte*  theData, udword theLength) {
  theConnection->transmitQueue = theData;
  theConnection->queueLength = theLength;
  theConnection->firstSeq = theConnection->sendNext;
  theConnection->theOffset = 0;
  theConnection->theFirst = 0;
  theConnection->theSendLength = 1460;
  // ??? SLIDING WINDOW FIX
  // Initialize transmission queue.

  theConnection->myTCPSender->sendFromQueue();
  //theConnection->sendNext += theLength; //??? WHAT Apparently seqnum refers to next BYTE not next segment
}


void EstablishedState::AppClose(TCPConnection* theConnection) {
  theConnection->myState=FinWait1State::instance();
  theConnection->myTCPSender->sendFlags(0x11);
  //send FIN ACK ???
  theConnection->sendNext += 1;
  cout << "EstablishedState -> FinWait1State" <<endl;
}




//----------------------------------------------------------------------------
//
CloseWaitState*
CloseWaitState::instance()
{
  static CloseWaitState myInstance;
  return &myInstance;
}




void CloseWaitState::AppClose(TCPConnection* theConnection) {
    theConnection->myState=LastAckState::instance();
    theConnection->myTCPSender->sendFlags(0x11);
    //send FIN ACK
    theConnection->sendNext += 1;
    cout << "hejdå" <<endl;
}





//----------------------------------------------------------------------------
//
LastAckState*
LastAckState::instance()
{
  static LastAckState myInstance;
  return &myInstance;
}



void LastAckState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber){
    if (theConnection->sendNext == theAcknowledgementNumber) {
        theConnection->Kill();
        cout << "deadd"<< endl;
    }
}

//----------------------------------------------------------------------------
//
FinWait1State*
FinWait1State::instance()
{
  static FinWait1State myInstance;
  return &myInstance;
}

void FinWait1State::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
    if (theConnection->sendNext == theAcknowledgementNumber) {
      theConnection->myState = FinWait2State::instance();
      cout << "FinWait1State -> FinWait2State" << endl;
    }
}
//----------------------------------------------------------------------------
//
FinWait2State*
FinWait2State::instance()
{
  static FinWait2State myInstance;
  return &myInstance;
}

void FinWait2State::NetClose(TCPConnection* theConnection, udword theSynchronizationNumber) {
  // Update connection variables and send an ACK
  theConnection -> receiveNext = theSynchronizationNumber +1;
  // ??? sequence check is bad?
  theConnection->myTCPSender->sendFlags(0x10);
  //Send ACK

  theConnection->Kill();
  //No time wait state, kill instantly.
}

//----------------------------------------------------------------------------
//

void TCPState::Synchronize(TCPConnection* theConnection,
                        udword theSynchronizationNumber) { cout << "should not happen" << endl;}
// Handle an incoming SYN segment
void TCPState::NetClose(TCPConnection* theConnection, udword theSynchronizationNumber) {
  cout << "should not happen 1" << endl;}
// Handle an incoming FIN segment
void TCPState::AppClose(TCPConnection* theConnection) {
  cout << "should not happen 2" << endl;}
// Handle close from application
void TCPState::Receive(TCPConnection* theConnection,
                    udword theSynchronizationNumber,
                    byte*  theData,
                    udword theLength) {
                      cout << "should not happen 3" << endl;}
// Handle incoming data
void TCPState::Acknowledge(TCPConnection* theConnection,
                        udword theAcknowledgementNumber) {
                          cout <<  theAcknowledgementNumber<< "<-- ackNbr"<<endl;
                          cout <<  HILO(theAcknowledgementNumber)<< "<-- ackNbr HILO"<<endl;
                          cout << "should not happen 4" << endl;}
// Handle incoming Acknowledgement
void TCPState::Send(TCPConnection* theConnection, byte*  theData, udword theLength) {
  cout << "should not happen 5" << endl;}


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
TCPSender::sendData(byte* theData, udword theLength)
{
  udword totalDataLength = Ethernet::ethernetHeaderLength + IP::ipHeaderLength + TCP::tcpHeaderLength + theLength;
  udword hoffs = Ethernet::ethernetHeaderLength + IP::ipHeaderLength;
  udword totalSegmentLength = TCP::tcpHeaderLength + theLength;
  // Decide on the value of the length totalSegmentLength.
  // Allocate a TCP segment.
  byte* anAnswer = new byte[totalDataLength];

  byte* dataPointer = anAnswer+hoffs+TCP::tcpHeaderLength;
  // cout << "addressOF anAnswer"<<(int) anAnswer[0] <<endl;
  // cout << "addressOF dataPointer"<<(int) dataPointer[0] <<endl;
  // cout << "addition" << (int)(hoffs+TCP::tcpHeaderLength) << endl;
  // cout << "valueAt anAnswer: "<< (int) anAnswer[0] << endl;
  //   cout << "valueAt dataPointer: "<< (int) dataPointer[0] << endl;
  //memcpy(dataPointer, theData, theLength);

  // cout << "valueAt anAnswer AFTER: "<< (int) anAnswer[0] << endl;
  // cout << "valueAt dataPointer AFTER: "<< (int) dataPointer[0] << endl;

  // Calculate the pseudo header checksum
  TCPPseudoHeader* aPseudoHeader =
    new TCPPseudoHeader(myConnection->hisAddress,
                        totalSegmentLength);
  uword pseudosum = aPseudoHeader->checksum();
  delete aPseudoHeader;
  // Create the TCP segment.

  TCPHeader* tcpHeader = (TCPHeader*) (anAnswer+hoffs);
  tcpHeader->sourcePort = HILO(myConnection->myPort);
  tcpHeader->destinationPort = HILO(myConnection->hisPort);
  tcpHeader->sequenceNumber = LHILO(myConnection->sendNext);
  tcpHeader->acknowledgementNumber = LHILO(myConnection->receiveNext);
  tcpHeader->headerLength = (TCP::tcpHeaderLength/4) * 16;
  // since it only occupies 4 bits
  tcpHeader->flags = 0x18;
  tcpHeader->windowSize = HILO(myConnection->receiveWindow);
  tcpHeader->urgentPointer = 0;
  tcpHeader->checksum = 0;
  memcpy(dataPointer, theData, theLength);

  // Calculate the final checksum.
  // cout << "HERE'S TCP: ";
  // for(int i = 0; i < totalSegmentLength; i++) {
  //   cout << hex << (int)((byte*)tcpHeader)[i] << ":";
  // }
  // cout << endl;

  //
  // for(int i = 0; i < theLength; i++) {
  //   cout << hex << (int)(byte*)(anAnswer+hoffs+TCP::tcpHeaderLength)[i] << ":";
  // }
  // cout << endl;
  //
  // cout << "The length " << (int)theLength << endl;

  tcpHeader->checksum = calculateChecksum((byte*)tcpHeader,
                                           totalSegmentLength,
                                           pseudosum);
  // Send the TCP segment.

  myAnswerChain->answer((byte*)tcpHeader,
                        totalSegmentLength);
  // Deallocate the dynamic memory
  // delete anAnswer;
}

bool TCPSender::sendFromQueue() {
  if(myConnection->theOffset == myConnection->queueLength) {
    //no more data in queue
    return false;
  }

  uword sendLength = (myConnection->queueLength - myConnection->theOffset) < myConnection->theSendLength
    ? myConnection->queueLength - myConnection->theOffset
    : myConnection->theSendLength;
    cout << "real sendLength:" << sendLength << endl;
    cout << "sendLength if condition: " << (myConnection->queueLength - myConnection->theOffset) << endl;
    cout << endl;

  // Either maxLength or remaining data length
  sendData(myConnection->transmitQueue + myConnection->theOffset, sendLength);
  myConnection->theOffset += sendLength;
  myConnection->sendNext += sendLength;

  //theFirst omitted for now.
  return true;
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
  TCPHeader* tcpHeader = (TCPHeader*) (anAnswer+hoffs);
  tcpHeader->sourcePort = HILO(myConnection->myPort);
  tcpHeader->destinationPort = HILO(myConnection->hisPort);
  tcpHeader->sequenceNumber = LHILO(myConnection->sendNext);
  tcpHeader->acknowledgementNumber = LHILO(myConnection->receiveNext);
  tcpHeader->headerLength = (TCP::tcpHeaderLength/4) * 16;
  // since it only occupies 4 bits
  tcpHeader->flags = theFlags;
  tcpHeader->windowSize = HILO(myConnection->receiveWindow);
  tcpHeader->urgentPointer = 0;
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
        // Synchronize updates state to SynRecvdState
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
      if ((tcpHeader->flags & 0x10) != 0 ) {
        // only ACK flag (0x10)
        ///handle close in this state ???
        // ??? require both SYN and ACK flags to be set?
        tcpConnection->Acknowledge(myAcknowledgementNumber);
      }

      //in EstablishedState:
      if((tcpHeader->flags & 0x01) != 0) {
        // FIN flag = 0x01
          tcpConnection->NetClose(mySequenceNumber);
      }
      /*
      if((tcpHeader->flags & 0x10) != 0 ) {
        //receive ack of FIN
        tcpConnection->Acknowledge(myAcknowledgementNumber);
      }
      */
      if((tcpHeader->flags & 0x8) != 0 ) {
        tcpConnection->Receive(mySequenceNumber, myData+TCP::tcpHeaderLength,
                                myLength - TCP::tcpHeaderLength);
      }


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
