/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    linuxtcpsocketserver.h
 * @date    17.07.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_LINUXTCPSOCKETSERVERCONNECTOR_H_
#define JSONRPC_CPP_LINUXTCPSOCKETSERVERCONNECTOR_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "../abstractthreadedserver.h"

namespace jsonrpc {
/**
 * This class is the Linux/UNIX implementation of TCPSocketServer.
 * It uses the POSIX socket API and POSIX thread API to performs its job.
 * Each client request is handled in a new thread.
 */
class LinuxTcpSocketServer : public AbstractThreadedServer {
public:
  /**
   * @brief LinuxTcpSocketServer, constructor of the Linux/UNIX
   * implementation of class TcpSocketServer
   * @param ipToBind The ipv4 address on which the server should
   * bind and listen
   * @param port The port on which the server should bind and listen
   */
  LinuxTcpSocketServer(const std::string &ipToBind, const unsigned int &port,
                       size_t threads = 1);

  virtual ~LinuxTcpSocketServer();

  virtual bool InitializeListener();
  virtual int CheckForConnection();
  virtual void HandleConnection(int connection);

private:
  std::string ipToBind;
  unsigned int port;
  int socket_fd;
  struct sockaddr_in address;

  /**
   * @brief A method that wait for the client to close the tcp session
   *
   * This method wait for the client to close the tcp session in order to avoid
   * the server to enter in TIME_WAIT status.
   * Entering in TIME_WAIT status with too many clients may occur in a DOS
   * attack
   * since server will not be able to use a new socket when a new client
   * connects.
   * @param fd The file descriptor of the socket that should be closed by the
   * client
   * @param timeout The maximum time the server will wait for the client to
   * close the tcp session in microseconds.
   * @returns A boolean indicating the success or the failure of the operation
   */
  bool WaitClientClose(const int &fd, const int &timeout = 100000);
  /**
   * @brief A method that close a socket by reseting it
   *
   * This method reset the tcp session in order to avoid enter in TIME_WAIT
   * state.
   * @param fd The file descriptor of the socket that should be reset
   * @returns The return value of POSIX close() method
   */
  int CloseByReset(const int &fd);
  /**
   * @brief A method that cleanly close a socket by avoid TIME_WAIT state
   *
   * This method uses WaitClientClose and ClodeByReset to clenly close a tcp
   * session with a client
   * (avoiding TIME_WAIT to avoid DOS attacks).
   * @param fd The file descriptor of the socket that should be cleanly closed
   * @returns The return value of POSIX close() method
   */
  int CleanClose(const int &fd);
};

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_LINUXTCPSOCKETSERVERCONNECTOR_H_ */
