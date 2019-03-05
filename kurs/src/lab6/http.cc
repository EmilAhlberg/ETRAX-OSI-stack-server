/*!***************************************************************************
*!
*! FILE NAME  : http.cc
*!
*! DESCRIPTION: HTTP, Hyper text transfer protocol.
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
#include "tcpsocket.hh"
#include "http.hh"
#include "fs.hh"

//#define D_HTTP
#ifdef D_HTTP
#define trace cout
#else
#define trace if(false) cout
#endif

void print(byte* data, udword length) {
  for(int i = 0; i<length; i++) {
    cout << data[i];
  }
  cout << endl;
}

HTTPServer::HTTPServer(TCPSocket* theSocket) : mySocket(theSocket) {

}

//TODO:
// 1. CTRL+R ?
// 2. HANG ON STARTUP?
// 3. TCP-KEEP-ALIVE
// 4. LEAKs
// 5. HEAD
// 6. retransmission-implementation (% 30)...

void HTTPServer::doit() {
  udword aLength;
  byte* aData;
  bool done = false;

  while (!done && !mySocket->isEof())
  {
    char* buffer = new char[10000];
    int readHeaderLength = 0;
    bool isReading = true;
    int currentLength = 0;
    //read header
    do {
      aData = mySocket->Read(aLength);

      currentLength = 0;
      for(int i = 0; i<aLength-3; i++) {
        if (aData[i] == 0x0D && aData[i+2] == 0x0D) {

          isReading = false;
          currentLength = i+4;
          break;
        }
      }
      if(isReading) {
        currentLength = aLength;
      }
      memcpy(buffer+readHeaderLength, aData, currentLength);
      readHeaderLength += currentLength;
      if(isReading) {
        delete[] aData;
        aLength = 0;
      }
    } while (isReading);

    int headerOffset = readHeaderLength;
    int readContentLength = aLength-currentLength;

    if(strncmp(buffer, "POST", 4) == 0) {
        //copy remaining data into buffer
        int contentLen = contentLength((char*)buffer, readHeaderLength);
        memcpy(buffer+readHeaderLength, aData+currentLength, aLength-currentLength);
        delete[] aData;

        //read content
        while (contentLen > readContentLength) {
          aData = mySocket->Read(aLength);

          memcpy((byte*)buffer+(headerOffset+readContentLength), aData, aLength);
          readContentLength += aLength;
          delete[] aData;
          aLength = 0;

        }
        /*
        for (int i = 0; i < headerOffset+readContentLength; i++) {
          cout << buffer[i];
        }*/
    }

    aData = buffer;
    aLength = headerOffset + readContentLength;


    if (aLength > 0)
    {
      udword parsedLength = 0;
      int methodLength = 3;
      //LEAK???
      char* requestLine = 0;
      char* lhaFilePath = 0;
      udword writeLength = 0;
      byte* returnFile = 0;
      int fileNameLength = 0;
      char* fileName = 0;
      int fileNameOffset = 0;
      int fileTypePos = 0;
      char* authData = 0;
      char* userName = 0;
      char* password = 0;
      int contentLen = 0;
      //handle one row at a time
      while(aData[parsedLength] != 0x0D && parsedLength < aLength) {

        //identify get method
        if (strncmp(aData+parsedLength, "GET", 3) == 0 || strncmp(aData+parsedLength, "POS", 3) == 0) {
          //cout << "GET recognized: " << endl;

          if(strncmp(aData+parsedLength, "POS", 3) == 0) {
            contentLen = contentLength((char*)aData, aLength);
            methodLength = 4;
          }

          for(int i = methodLength+1; i<aLength; i++) {
            if (aData[i] == 0x0D) {

              requestLine = extractString((char*)aData, i);
              //cout << "header" << endl;
              //print(requestLine, i);
              fileName = extractString((char*)(aData+fileNameOffset), fileNameLength);
              //fileName = "pict/small1.gif";
              lhaFilePath = findPathName((char*)requestLine);
              if (lhaFilePath == NULL && fileNameLength == 0) {
                fileName = "index.htm";
                fileNameLength = 9;
              }
              //cout << "fileName:" << endl;
              //print(fileName, fileNameLength);
              returnFile = FileSystem::instance().readFile(lhaFilePath, fileName, writeLength);
              //cout << "Returnfile:" << endl;
              //print(returnFile, writeLength);
              break;
            }
            if (aData[i] == 0x20) {
              fileNameLength = i - fileNameOffset;
            }
            if(aData[i] == '/' && fileNameLength == 0) {
              fileNameOffset = i + 1 ; // +1 trims slash
            }
            if(aData[i] == '.' && fileNameLength == 0) {
              fileTypePos = i+1;
            }
          }
        }
        //identify POST



        //identify authentication field
        if (strncmp(aData+parsedLength, "Author", 6) == 0) {
          for(int i = parsedLength+21; i<aLength; i++) {
              if (aData[i] == 0x0D) {
                authData = extractString((char*)aData+(parsedLength+21),i);
                //cout << endl;
                break;
              }
          }
          char* authDecoded = decodeBase64(authData);
          int splitIndex = strlen(authDecoded);

          for(int i=0; i < strlen(authDecoded); i++) { //!!! aLength should be length of string
            if(authDecoded[i] == ':') {
              splitIndex = i;
              break;
            }
          }

          userName = extractString(authDecoded, splitIndex);
          //print(userName, splitIndex);

          password = extractString(authDecoded+splitIndex+1, strlen(authDecoded)-splitIndex);
          //print(password, strlen(authDecoded)-splitIndex);

          //do auth stuff
        }
        for(int i = parsedLength; i<aLength; i++) {
          //row end condition
          if (aData[i] == 0x0D) {
            //add recently taken steps and skip CR + LF
            parsedLength += i-parsedLength + 1 + 1;
            break;
          }
          //cout << aData[i];
        }

      }
      parsedLength += 2; // to pass the last CRLF

      //write data

      if(contentLen > 0) {
        char* contentString = extractString((char*)aData+headerOffset, readContentLength);
        char* parsedContentString = decodeForm(contentString);
        //print(parsedContentString, strlen(parsedContentString));
        FileSystem::instance().writeFile(0, 0, (byte*)parsedContentString, strlen(parsedContentString));
      }

      char* headerLine = 0;
      int headerLength = 0;

      switch(aData[fileTypePos]) {
        case 'g':
          headerLine = "Content-Type: image/gif\x0D\x0A\x0D\x0A";
          headerLength = 27;
          break;
        case 'j':
          headerLine = "Content-Type: image/jpeg\x0D\x0A\x0D\x0A";
          headerLength = 28;
          break;
        default:
          headerLine = "Content-Type: text/html\x0D\x0A\x0D\x0A";
          headerLength = 27;
          break;
        }

      char* responseLine = "HTTP/1.0 200 OK\x0D\x0A";
      int responseLineLength  = 17;

      char* pathString = extractString(lhaFilePath, strlen(lhaFilePath));
      //print(pathString, 7);

      if (returnFile == NULL) {
        responseLine = "HTTP/1.0 404 Not found\x0D\x0A";
        responseLineLength = 24;
        headerLine = "Content-Type: text/html\x0D\x0A\x0D\x0A";
        headerLength = 27;
        returnFile = "<html><head><title>File not found</title></head><body><h1>404 Not found</h1></body></html>";
        writeLength = strlen((char*)returnFile);
      }
      else
      if (contentLen > 0) {
        responseLine = "HTTP/1.0 200 OK\x0D\x0A";
        responseLineLength  = 17;
        headerLine = "Content-Type: text/html\x0D\x0A\x0D\x0A";
        headerLength = 27;
        returnFile = "<html><head><title>Accepted</title></head><body><h1>The file dynamic.htm was updated successfully.</h1></body></html>";
        writeLength = strlen((char*)returnFile);
      }
      else
      if(strncmp(pathString, "private", 7) == 0) {
        if (!(strncmp(userName, "emil", 4) == 0 && strncmp(password, "oskar", 5) == 0)) {
          responseLine = "HTTP/1.0 401 Unauthorized\x0D\x0A";
          responseLineLength = 27;
          headerLine = "Content-Type: text/html\x0D\x0AWWW-Authenticate: Basic realm=\"private\"\x0D\x0A\x0D\x0A";
          headerLength = 68;
          returnFile = "<html><head><title>401 Unauthorized</title></head>\x0D\x0A<body><h1>401 Unauthorized</h1></body></html>";
          writeLength = strlen((char*)returnFile);
        }
      }


      mySocket->Write((byte*)responseLine, responseLineLength);
      mySocket->Write((byte*)headerLine, headerLength);
      mySocket->Write((byte*)returnFile, writeLength);

      done = true;

      delete[] requestLine;
      delete[] lhaFilePath;
      delete[] returnFile;
      delete[] fileName;
      delete[] headerLine;
      delete[] responseLine;
      delete[] aData;
    }
  }
  mySocket->Close();
}

/*
//print row until <CR>
for(int i = parsedLength; i<aLength; i++) {
  //row end condition
  if (aData[i] == 0x0D) {
    //add recently taken steps and skip CR + LF
    parsedLength += i-parsedLength + 1 + 1;
    row++;
    break;
  }
  cout << (char) aData[i];
}
cout << endl;

*/

/****************** HTTPServer DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//
// Allocates a new null terminated string containing a copy of the data at
// 'thePosition', 'theLength' characters long. The string must be deleted by
// the caller.
//
char*
HTTPServer::extractString(char* thePosition, udword theLength)
{
  char* aString = new char[theLength + 1];
  strncpy(aString, thePosition, theLength);
  aString[theLength] = '\0';
  return aString;
}

char*
HTTPServer::findPathName(char* str)
{
  char* firstPos = strchr(str, ' ');     // First space on line
  firstPos++;                            // Pointer to first /
  char* lastPos = strchr(firstPos, ' '); // Last space on line
  char* thePath = 0;                     // Result path
  if ((lastPos - firstPos) == 1)
  {
    // Is / only
    thePath = 0;                         // Return NULL
  }
  else
  {
    // Is an absolute path. Skip first /.
    thePath = extractString((char*)(firstPos+1),
                            lastPos-firstPos);
    if ((lastPos = strrchr(thePath, '/')) != 0)
    {
      // Found a path. Insert -1 as terminator.
      *lastPos = '\xff';
      *(lastPos+1) = '\0';
      while ((firstPos = strchr(thePath, '/')) != 0)
      {
        // Insert -1 as separator.
        *firstPos = '\xff';
      }
    }
    else
    {
      // Is /index.html
      delete thePath; thePath = 0; // Return NULL
    }
  }
  return thePath;
}

//----------------------------------------------------------------------------
//
// Will look for the 'Content-Length' field in the request header and convert
// the length to a udword
// theData is a pointer to the request. theLength is the total length of the
// request.
//
udword
HTTPServer::contentLength(char* theData, udword theLength)
{
  udword index = 0;
  bool   lenFound = false;
  const char* aSearchString = "Content-Length: ";
  while ((index++ < theLength) && !lenFound)
  {
    lenFound = (strncmp(theData + index,
                        aSearchString,
                        strlen(aSearchString)) == 0);
  }
  if (!lenFound)
  {
    return 0;
  }
  trace << "Found Content-Length!" << endl;
  index += strlen(aSearchString) - 1;
  char* lenStart = theData + index;
  char* lenEnd = strchr(theData + index, '\r');
  char* lenString = this->extractString(lenStart, lenEnd - lenStart);
  udword contLen = atoi(lenString);
  trace << "lenString: " << lenString << " is len: " << contLen << endl;
  delete [] lenString;
  return contLen;
}

//----------------------------------------------------------------------------
//
// Decode user and password for basic authentication.
// returns a decoded string that must be deleted by the caller.
//
char*
HTTPServer::decodeBase64(char* theEncodedString)
{
  static const char* someValidCharacters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  int aCharsToDecode;
  int k = 0;
  char  aTmpStorage[4];
  int aValue;
  char* aResult = new char[80];

  // Original code by JH, found on the net years later (!).
  // Modify on your own risk.

  for (unsigned int i = 0; i < strlen(theEncodedString); i += 4)
  {
    aValue = 0;
    aCharsToDecode = 3;
    if (theEncodedString[i+2] == '=')
    {
      aCharsToDecode = 1;
    }
    else if (theEncodedString[i+3] == '=')
    {
      aCharsToDecode = 2;
    }

    for (int j = 0; j <= aCharsToDecode; j++)
    {
      int aDecodedValue;
      aDecodedValue = strchr(someValidCharacters,theEncodedString[i+j])
        - someValidCharacters;
      aDecodedValue <<= ((3-j)*6);
      aValue += aDecodedValue;
    }
    for (int jj = 2; jj >= 0; jj--)
    {
      aTmpStorage[jj] = aValue & 255;
      aValue >>= 8;
    }
    aResult[k++] = aTmpStorage[0];
    aResult[k++] = aTmpStorage[1];
    aResult[k++] = aTmpStorage[2];
  }
  aResult[k] = 0; // zero terminate string

  return aResult;
}

//------------------------------------------------------------------------
//
// Decode the URL encoded data submitted in a POST.
//
char*
HTTPServer::decodeForm(char* theEncodedForm)
{
  char* anEncodedFile = strchr(theEncodedForm,'=');
  anEncodedFile++;
  char* aForm = new char[strlen(anEncodedFile) * 2];
  // Serious overkill, but what the heck, we've got plenty of memory here!
  udword aSourceIndex = 0;
  udword aDestIndex = 0;

  while (aSourceIndex < strlen(anEncodedFile))
  {
    char aChar = *(anEncodedFile + aSourceIndex++);
    switch (aChar)
    {
     case '&':
       *(aForm + aDestIndex++) = '\r';
       *(aForm + aDestIndex++) = '\n';
       break;
     case '+':
       *(aForm + aDestIndex++) = ' ';
       break;
     case '%':
       char aTemp[5];
       aTemp[0] = '0';
       aTemp[1] = 'x';
       aTemp[2] = *(anEncodedFile + aSourceIndex++);
       aTemp[3] = *(anEncodedFile + aSourceIndex++);
       aTemp[4] = '\0';
       udword anUdword;
       anUdword = strtoul((char*)&aTemp,0,0);
       *(aForm + aDestIndex++) = (char)anUdword;
       break;
     default:
       *(aForm + aDestIndex++) = aChar;
       break;
    }
  }
  *(aForm + aDestIndex++) = '\0';
  return aForm;
}

/************** END OF FILE http.cc *************************************/
