/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    abstractserverconnector.h
 * @date    31.12.2012
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_SERVERCONNECTOR_H_
#define JSONRPC_CPP_SERVERCONNECTOR_H_

#include "iclientconnectionhandler.h"
#include <string>

namespace jsonrpc {

class AbstractServerConnector {
public:
  AbstractServerConnector();
  virtual ~AbstractServerConnector();

  /**
   * This method should signal the Connector to start waiting for requests, in
   * any way that is appropriate for the derived connector class.
   * If something went wrong, this method should return false, otherwise true.
   */
  virtual bool StartListening() = 0;
  /**
   * This method should signal the Connector to stop waiting for requests, in
   * any way that is appropriate for the derived connector class.
   * If something went wrong, this method should return false, otherwise true.
   */
  virtual bool StopListening() = 0;

  void ProcessRequest(const std::string &request, std::string &response);

  void SetHandler(IClientConnectionHandler *handler);
  IClientConnectionHandler *GetHandler();

private:
  IClientConnectionHandler *handler;
};

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_ERVERCONNECTOR_H_ */
