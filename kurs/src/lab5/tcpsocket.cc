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
        byte* queue = new byte[5*4000];
        for (int i = 0; i < 5*100; i++) {
          memcpy(queue+(i*40), str, 40);
        }
        mySocket->Write(queue, 5*4000);

        // for (int i = 0; i < 5*4000; i++) {
        //   cout << hex << (char)queue[i];
        // }
        // cout << endl;

        delete str; //???????????? 
        delete queue;
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
  myConnection->Send(theData, theLength);
  myWriteSemaphore->wait(); // Wait until the data is acknowledged
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
