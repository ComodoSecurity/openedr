/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    windowstcpsocketclient.cpp
 * @date    17.07.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "windowstcpsocketclient.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#include <ws2tcpip.h>

#define BUFFER_SIZE 64
#ifndef DELIMITER_CHAR
#define DELIMITER_CHAR char(0x0A)
#endif // DELIMITER_CHAR

using namespace jsonrpc;
using namespace std;

WindowsTcpSocketClient::WindowsTcpSocketClient(const std::string &hostToConnect,
                                               const unsigned int &port)
    : hostToConnect(hostToConnect), port(port), socket_(INVALID_SOCKET) {}

WindowsTcpSocketClient::~WindowsTcpSocketClient() { closesocket(socket_);  }

void WindowsTcpSocketClient::SendRPCMessage(
    const std::string &message, std::string &result) throw(JsonRpcException) {
  SOCKET socket_fd = this->Connect();
  char buffer[BUFFER_SIZE];
  bool fullyWritten = false;
  string toSend = message;
  do {
    size_t byteWritten = send(socket_fd, toSend.c_str(), toSend.size(), 0);
    if (byteWritten == -1) {
      string message = "send() failed";
      int err = WSAGetLastError();
      switch (err) {
      case WSANOTINITIALISED:
      case WSAENETDOWN:
      case WSAEACCES:
      case WSAEINTR:
      case WSAEINPROGRESS:
      case WSAEFAULT:
      case WSAENETRESET:
      case WSAENOBUFS:
      case WSAENOTCONN:
      case WSAENOTSOCK:
      case WSAEOPNOTSUPP:
      case WSAESHUTDOWN:
      case WSAEWOULDBLOCK:
      case WSAEMSGSIZE:
      case WSAEHOSTUNREACH:
      case WSAEINVAL:
      case WSAECONNABORTED:
      case WSAECONNRESET:
      case WSAETIMEDOUT:
        message = GetErrorMessage(err);
        break;
      }
      closesocket(socket_fd);
      throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR, message);
    } else if (static_cast<unsigned int>(byteWritten) < toSend.size()) {
      size_t len = toSend.size() - byteWritten;
      toSend = toSend.substr(byteWritten + sizeof(char), len);
    } else
      fullyWritten = true;
  } while (!fullyWritten);

  do {
    int nbytes = recv(socket_fd, buffer, BUFFER_SIZE, 0);
    if (nbytes == -1) {
      string message = "recv() failed";
      int err = WSAGetLastError();
      switch (err) {
      case WSANOTINITIALISED:
      case WSAENETDOWN:
      case WSAEFAULT:
      case WSAENOTCONN:
      case WSAEINTR:
      case WSAEINPROGRESS:
      case WSAENETRESET:
      case WSAENOTSOCK:
      case WSAEOPNOTSUPP:
      case WSAESHUTDOWN:
      case WSAEWOULDBLOCK:
      case WSAEMSGSIZE:
      case WSAEINVAL:
      case WSAECONNABORTED:
      case WSAETIMEDOUT:
      case WSAECONNRESET:
        message = GetErrorMessage(err);
        break;
      }
      closesocket(socket_fd);
      throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR, message);
    } else {
      string tmp;
      tmp.append(buffer, nbytes);
      result.append(buffer, nbytes);
    }

  } while (result.find(DELIMITER_CHAR) == string::npos);

  // socket
  // closesocket(socket_fd);
}

string WindowsTcpSocketClient::GetErrorMessage(const int &e) {
	string message = "Unknown error";

	const WCHAR* lpMsgBuf;
	DWORD dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(WCHAR*)&lpMsgBuf, 0, NULL);

	// if not error
	if (dwResult != 0)
	{
		// convert to utf8
		wstring wstr = lpMsgBuf;
		LocalFree((HLOCAL)lpMsgBuf);

		if (!wstr.empty())
		{
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
			string u8str(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &u8str[0], size_needed, NULL, NULL);
			message = u8str;
		}
	}

	return message;
}

SOCKET WindowsTcpSocketClient::Connect() throw(JsonRpcException) {
  if (IsSocketActive())
    return socket_;
  else
	closesocket(socket_);
  if (this->IsIpv4Address(this->hostToConnect)) {
    return socket_ = this->Connect(this->hostToConnect, this->port);
  } else // We were given a hostname
  {
    struct addrinfo *result = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    char port[6];
    _itoa_s(this->port, port, 10);
    DWORD retval =
        getaddrinfo(this->hostToConnect.c_str(), port, &hints, &result);
    if (retval != 0)
      throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                             "Could not resolve hostname.");

    bool foundValidIp = false;
    SOCKET socket_fd = INVALID_SOCKET;
    for (struct addrinfo *temp = result; (temp != NULL) && !foundValidIp;
         temp = temp->ai_next) {
      if (temp->ai_family == AF_INET) {
        try {
          SOCKADDR_IN *sock = reinterpret_cast<SOCKADDR_IN *>(temp->ai_addr);
          socket_fd =
              this->Connect(inet_ntoa(sock->sin_addr), ntohs(sock->sin_port));
          foundValidIp = true;
        } catch (const JsonRpcException &e) {
          foundValidIp = false;
          socket_fd = INVALID_SOCKET;
        } catch (void *p) {
          foundValidIp = false;
          socket_fd = INVALID_SOCKET;
        }
      }
    }

    if (!foundValidIp)
      throw JsonRpcException(
          Errors::ERROR_CLIENT_CONNECTOR,
          "Hostname resolved but connection was refused on the given port.");

    return socket_ = socket_fd;
  }
}

SOCKET
WindowsTcpSocketClient::Connect(const string &ip,
                                const int &port) throw(JsonRpcException) {
  SOCKADDR_IN address;
  SOCKET socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd == INVALID_SOCKET) {
    string message = "socket() failed";
    int err = WSAGetLastError();
    switch (err) {
    case WSANOTINITIALISED:
    case WSAENETDOWN:
    case WSAEAFNOSUPPORT:
    case WSAEINPROGRESS:
    case WSAEMFILE:
    case WSAEINVAL:
    case WSAEINVALIDPROVIDER:
    case WSAEINVALIDPROCTABLE:
    case WSAENOBUFS:
    case WSAEPROTONOSUPPORT:
    case WSAEPROTOTYPE:
    case WSAEPROVIDERFAILEDINIT:
    case WSAESOCKTNOSUPPORT:
      message = GetErrorMessage(err);
      break;
    }
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR, message);
  }
  memset(&address, 0, sizeof(SOCKADDR_IN));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip.c_str());
  address.sin_port = htons(port);
  if (connect(socket_fd, reinterpret_cast<SOCKADDR *>(&address),
              sizeof(SOCKADDR_IN)) != 0) {
    string message = "connect() failed";
    int err = WSAGetLastError();
    switch (err) {
    case WSANOTINITIALISED:
    case WSAENETDOWN:
    case WSAEADDRINUSE:
    case WSAEINTR:
    case WSAEINPROGRESS:
    case WSAEALREADY:
    case WSAEADDRNOTAVAIL:
    case WSAEAFNOSUPPORT:
    case WSAECONNREFUSED:
    case WSAEFAULT:
    case WSAEINVAL:
    case WSAEISCONN:
    case WSAENETUNREACH:
    case WSAEHOSTUNREACH:
    case WSAENOBUFS:
    case WSAENOTSOCK:
    case WSAETIMEDOUT:
    case WSAEWOULDBLOCK:
    case WSAEACCES:
      message = GetErrorMessage(err);
      break;
    }
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR, message);
  }
  return socket_fd;
}

bool WindowsTcpSocketClient::IsSocketActive()
{
	int optVal;
	int optLen = sizeof(int);
	return ((socket_ != INVALID_SOCKET) && (SOCKET_ERROR != getsockopt(socket_, SOL_SOCKET, SO_ACCEPTCONN,
		(char*)&optVal, &optLen)));
}

bool WindowsTcpSocketClient::IsIpv4Address(const std::string &ip) {
  return (inet_addr(ip.c_str()) != INADDR_NONE);
}

// This is inspired from SFML to manage Winsock initialization. Thanks to them!
// ( http://www.sfml-dev.org/ ).
struct ClientSocketInitializer {
  ClientSocketInitializer()

  {
    WSADATA init;
    if (WSAStartup(MAKEWORD(2, 2), &init) != 0) {
      throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                             "An issue occurred while WSAStartup executed.");
    }
  }

  ~ClientSocketInitializer()

  {
    if (WSACleanup() != 0) {
      cerr << "An issue occurred while WSAClean executed." << endl;
    }
  }
};

struct ClientSocketInitializer clientGlobalInitializer;
