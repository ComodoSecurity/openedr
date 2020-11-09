/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    xbmcremote.cpp
 * @date    03.06.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "gen/xbmcremote.h"
#include <jsonrpccpp/client/connectors/httpclient.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#include <termios.h>
#else
#include <conio.h>
#endif
#include <time.h>
#include <unistd.h>

#include <iostream>

using namespace jsonrpc;
using namespace std;

// Taken from:
// http://stackoverflow.com/questions/2984307/c-key-pressed-in-linux-console
int kbhit() {
  int ch;
#ifndef WIN32
  struct termios neu, alt;
  int fd = fileno(stdin);
  tcgetattr(fd, &alt);
  neu = alt;
  neu.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(fd, TCSANOW, &neu);
  ch = getchar();
  tcsetattr(fd, TCSANOW, &alt);
#else
  while (!_kbhit()) {
    usleep(100000);
  }
  ch = _getch();
#endif
  return ch;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    cerr << "Provide XBMC API URL as argument! e.g.: " << argv[0]
         << " http://127.0.0.1:8080/jsonrpc" << endl;
    return -1;
  } else {
    cout << "XBMC Remote control" << endl;
    cout << "\ta -> left" << endl;
    cout << "\td -> right" << endl;
    cout << "\tw -> up" << endl;
    cout << "\td -> down" << endl;
    cout << "\tEsc -> back" << endl;
    cout << "\tEnter -> select" << endl;
    cout << "\tx -> exit application" << endl;

    try {
      HttpClient httpclient(argv[1]);
      XbmcRemoteClient stub(httpclient);
      bool run = true;
      while (run) {

        int key = kbhit();
        switch (key) {
        case 97:
          stub.Input_Left();
          break;
        case 115:
          stub.Input_Down();
          break;
        case 100:
          stub.Input_Right();
          break;
        case 119:
          stub.Input_Up();
          break;
        case 10:
        case 13:
          stub.Input_Select();
          break;
        case 127:
        case 27:
          stub.Input_Back();
          break;
        case 120:
          run = false;
          break;
        }
      }
    } catch (JsonRpcException &e) {
      cerr << e.what() << endl;
    }
  }
  return 0;
}
