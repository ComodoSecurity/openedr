//
// EDRAv2.libcore project.
//
// Author: Yury Ermakov (30.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//
///
/// @file JSON-RPC server declaration
///
/// @addtogroup ipc Inter-process communication
/// @{
#pragma once
#include <command.hpp>
#include <objects.h>

namespace cmd {
namespace ipc {

// Forward declaration
namespace detail {
class Server;
} // namespace detail

///
/// JSON-RPC server object class.
///
class JsonRpcServer : public ObjectBase<CLSID_JsonRpcServer>, 
	public ICommandProcessor
{
private:
	std::unique_ptr<jsonrpc::HttpServer> m_pHttpServer;
	std::unique_ptr<detail::Server> m_pServer;
	std::mutex m_mtxServer;

protected:
	///
	/// Constructor.
	///
	JsonRpcServer();
	
	///
	/// Destructor.
	///
	virtual ~JsonRpcServer();

	//
	//
	//
	void startServer();

	//
	//
	//
	void stopServer();

public:
	
	///
	/// Object final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **port** - port.
	///   **processor** - object to process commands.
	///   **numThreads** - (Optional) server's working thread pool size. If 
	///     numThreads equals 0 (by default), one thread per connection is used,
	///     instead of thread pool.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace ipc
} // namespace cmd

/// @}
