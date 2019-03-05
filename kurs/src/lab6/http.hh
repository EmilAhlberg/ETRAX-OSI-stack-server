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

class HTTPServer : public Job
{
 public:
  HTTPServer(TCPSocket* theSocket);

  void doit();

  char* extractString(char* thePosition, udword theLength);

  char* findPathName(char* str);

  udword contentLength(char* theData, udword theLength);

  char* decodeBase64(char* theEncodedString);

  char* decodeForm(char* theEncodedForm);

 private:
  TCPSocket* mySocket;
  // Pointer to the application associated with this job.
};
