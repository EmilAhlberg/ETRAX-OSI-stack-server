#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
#include "timr.h"
#include "sp_alloc.h"
}

#include "iostream.hh"
#include "tcp.hh"
#include "ip.hh"
#include "ethernet.hh"
#include "tcpsocket.hh"
#include "threads.hh"



SimpleApplication::SimpleApplication(TCPSocket* theSocket) : mySocket(theSocket) {

}

void
SimpleApplication::doit()
{
  udword aLength;
  byte* aData;
  bool done = false;
  while (!done && !mySocket->isEof())
  {
    aData = mySocket->Read(aLength);
    if (aLength > 0)
    {
      mySocket->Write(aData, aLength);
      if ((char)*aData == 'q')
      {
        done = true;
      }
      if ((char)*aData == 's')
      {
        char* str = new char[40];
        for (char i = 0; i < 40; i++) {
          str[i] = 65+i;
        }
        //byte* queue = new byte[4000];
        byte* queue = (byte*) malloc(10000);
        for (int i = 0; i < 250; i++) {
          memcpy(queue+(i*40), str, 40);
        }
        mySocket->Write(queue, 10000);
        // for (int i = 0; i < 5*4000; i++) {
        //   cout << hex << (char)queue[i];
        // }
        // cout << endl;

        delete[] str; //????????????
        free(queue); //??
      }
      if ((char)*aData == 'r')
      {

        //byte* queue = new byte[4000];
        byte* queue = new byte[1000000];
        if(!queue) {
          cout << "Not enough memory for 1Mb queue" << endl;
          continue;
        }

        for (int i = 0; i < 25000; i++) {
          for (char j = 0; j < 40; j++) {
            queue[i*40+j] = j+65;
          }
        }
        mySocket->Write(queue, 1000000);
        // for (int i = 0; i < 5*4000; i++) {
        //   cout << hex << (char)queue[i];
        // }
        // cout << endl;

        delete[] queue; //??
      }
      delete aData;
    }
  }
  mySocket->Close();
}

//-----------------------------
//TCPSOCKET
//-----------------------------

TCPSocket::TCPSocket(TCPConnection* theConnection) : myConnection(theConnection) {
  myReadSemaphore = Semaphore::createQueueSemaphore("read", 0);
  myWriteSemaphore = Semaphore::createQueueSemaphore("write", 0);
  eofFound = false;
}

TCPSocket::~TCPSocket() {
  delete myReadSemaphore;
  delete myWriteSemaphore;
}

byte*
TCPSocket::Read(udword& theLength)
{
  myReadSemaphore->wait(); // Wait for available data
  theLength = myReadLength;
  byte* aData = myReadData;
  myReadLength = 0;
  myReadData = 0;
  return aData;
}

void
TCPSocket::socketDataReceived(byte* theData, udword theLength)
{
  myReadData = new byte[theLength];
  /// LEAK???
  memcpy(myReadData, theData, theLength);
  myReadLength = theLength;
  myReadSemaphore->signal(); // Data is available
}

void
TCPSocket::Write(byte* theData, udword theLength)
{
  if(myConnection->myState == EstablishedState::instance()) {
    myConnection->Send(theData, theLength);
    myWriteSemaphore->wait(); // Wait until the data is acknowledged
  }
  else {
    cout << "send from wrong state!" << endl;
  }
}

void
TCPSocket::socketDataSent()
{
  myWriteSemaphore->signal(); // The data has been acknowledged VERY IMPORTANT
}

void
TCPSocket::socketEof()
{
  eofFound = true;
  myReadSemaphore->signal();
}

void  TCPSocket::Close() {
  myConnection->AppClose();
  //Strange instructions
}

bool  TCPSocket::isEof() {
    return eofFound;
}
