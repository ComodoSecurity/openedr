//
// edrav2.libcore
// 
// Author: Denis Bogdanov (11.04.2019)
// Reviewer: Yury Ermakov (16.04.2019)
//
/// @file Declarations of base network facilities
/// @addtogroup cloudNetwork Generic network support
/// @{
///
#pragma once

namespace cmd {
namespace net {

///
/// Get information about all installed network adaptors (real or virtual).
///
/// @param fOnlyOnline - return only connected adapters.
/// @param fOnlyWithAssignedIp [opt] - return only adapters with IP address (default: false).
/// @param fSafe [opt] - don't return error in case of system calls (only system calls) (default: false).
/// @returns The function returns a sequence of data for each installed network adapter.
///
Sequence getNetworkAdaptorsInfo(bool fOnlyOnline,
	bool fOnlyWithAssignedIp = false, bool fSafe = false);

///
/// Parse cannonical URI and return variant with the following fields.
///
/// @param sUrl - URI text string.
/// @returns The function returns a dictionary with the following fields:
///		* scheme - using scheme (protocol).
///		* host [str] - URI host.
///		* port [str] - URL port.
///		* path [str] - URI hierarchical path.
///		* useSSL [bool] - flag of SSL usage.
///
Variant parseUrl(const std::string& sUrl);

///
/// Create cannonical URI string from declarative Variant description.
///
/// See parseUrl() function.
///
std::string combineUrl(Variant vUrl);

///
/// Class for performing basic HTTP requests.
///
class HttpClient
{
private:
	class Impl;
	typedef std::shared_ptr<Impl> ImplPtr;
	ImplPtr m_pImpl;

	void checkImplIsValid(SourceLocation sl);

public:
	HttpClient(const HttpClient&) = delete;
	HttpClient& operator=(const HttpClient&) = delete;
	~HttpClient();

	HttpClient(HttpClient&&) noexcept;
	HttpClient& operator=(HttpClient&&) noexcept;
	
	///
	/// Constructor.
	///
	/// @param sAddress - root URL. It can specify common address part for further requests
	/// @param vParams - a dictionary with common parameters for all requests:
	///    * triesCount [int, opt] - tries count in case of connection error (default: 1).
	///    * timeout [int, opt] - a timeout (in milliseconds) between tries (default: 1000ms).
	///    * checkShutdown [bool, opt] - don't process timeouts in case of application shutdown (default: true).
	///    * userAgent [str] - user agent string for all requests.
	///    * maxResponseSize [str, opt] - max size (in bytes) of a response body (default: 20MB).
	///    * operationTimeout [int, opt] - a default timeout (in milliseconds) for operation (default: 10000ms).
	///
	HttpClient(const std::string& sAddress = "", Variant vParams = Dictionary());
	
	///
	/// Constructor.
	///
	/// @param vAddress - root address description as a Variant (see parseUrl()).
	/// @param vParams - common parameters for all requests.
	///
	struct CallTag {};
	template<typename T, IsVariantType<T, int> = 0 >
	HttpClient(T vAddress, Variant vParams) : 
		HttpClient(CallTag(), vAddress, vParams)
	{
	}

	///
	/// Internal constructor.
	///
	HttpClient(CallTag, Variant vAddress, Variant vParams);

	///
	/// Sets the rool url.
	///
	/// @param sAddress - root URL. It can specify common address part for further requests.
	///
	void setRootUrl(const std::string& sAddress);

	///
	/// Set http parameters.
	///
	/// @param vParams - global parameters (see constructor vParams).
	///
	void setParams(Variant vParams);

	///
	/// Performs HTTP GET request and returns data.
	///
	/// @param sResource [opt] - Full address or relative path (relating to a root 
	///        path in the constructor) (default: "").
	/// @param vParams [opt] - options for current request (default: blank).
	/// @return The function returns a dictionary for JSON replays and Stream for
	///         other content types.
	///
	Variant get(const std::string& sResource = "", Variant vParams = Dictionary{});


	///
	/// Performs HTTP GET request and returns data.
	///
	/// @param sResource - full address or relative path (relating to a root 
	///		path in the constructor).
	/// @param pOutStream - stream for output data.
	/// @param vParams [opt] - options for current request.
	/// @return The function returns a dictionary for JSON replays and Stream for
	///         other content types.
	///
	Variant get(const std::string& sResource, ObjPtr<io::IWritableStream> pOutStream,
		Variant vParams = Dictionary{});

	///
	/// Performs HTTP HEAD request and returns data.
	///
	/// @param sResource [opt] - full address or relative path (relating to a root 
	///		path in the constructor) (default: "").
	/// @param vParams [opt] - options for current request (default: blank).
	/// @return The function returns a dictionary with the following fields:
	///		* contentLength - HTTP Content-lenght variable.
	///		* contentType - HTTP Content-type variable.
	///
	Variant head(const std::string& sResource = "", Variant vParams = Dictionary{});

	///
	/// Performs HTTP POST request and returns data.
	///
	/// @param sResource - full address or relative path (relating to a root 
	///        path in the constructor).
	/// @param vData - data for sending (can be stream, string or dictionary)
	/// @param vParams [opt] - options for current request (default: blank)
	/// @return The function returns a dictionary for JSON replays and Stream for
	///		other content types.
	///
	Variant post(const std::string& sResource, Variant vData, Variant vParams = Dictionary{});

	///
	/// Performs HTTP POST request and returns data.
	///
	/// @param vData - data for sending (can be stream, string or dictionary).
	/// @param vHdrParams [opt] - options for current request (default: blank).
	/// @return The function returns a dictionary for JSON replays and Stream for
	///		other content types.
	///
	Variant post(Variant vData, Variant vHdrParams = Dictionary{});

	///
	/// Performs HTTP GET request and download file by specified path.
	///
	/// @filePath - a full path where the downloaded file will be placed to.
	/// @param sResource [opt] - full address or relative path (relating to a root 
	///		path in the constructor) (default: "").
	/// @param vParams [opt] - header options for current request (default: blank).
	///
	void download(std::filesystem::path filePath, const std::string& sResource = "",
		Variant vParams = Dictionary{});

	///
	/// Abort the current operation.
	///
	void abort();

	///
	/// Restart after abort.
	///
	void restart();
};

namespace detail {
void setSoketOption(boost::asio::detail::socket_type socket, int nOptName, const void *pOptVal, int nOptLen);
} // namespace detail

///
/// Wrapper of setsockopt() for boost::asio::*::socket.
///
/// @param socket - socket object.
/// @param nOptionName - "optname" parameter of setsockopt().
/// @param optionValue - passed value.
/// @throw exception on error.
///
template<typename Socket, typename OptionValue>
inline void setSoketOption(Socket& socket, int nOptionName, const OptionValue& optionValue)
{
	detail::setSoketOption(socket.native_handle(),
		nOptionName, (const char*)&optionValue, sizeof(optionValue));
}

///
/// Wrapper of setsockopt() for boost::asio::*::socket.
///
/// @param socket - socket object.
/// @param nSendTimeoutMs - send timeout in milliseconds.
/// @param nReceiveTimeoutMs - receive timeout in milliseconds.
///
template<typename Socket>
inline void setSoketTimeout(Socket& socket, std::optional<Size> nSendTimeoutMs,
	std::optional<Size> nReceiveTimeoutMs)
{
	if (nSendTimeoutMs)
		setSoketOption(socket, SO_SNDTIMEO, (DWORD)nSendTimeoutMs.value());
	if (nReceiveTimeoutMs)
		setSoketOption(socket, SO_RCVTIMEO, (DWORD)nReceiveTimeoutMs.value());
}

///
/// Get IP addresses which belong to a specified host by its name.
///
/// @param sHostname - a name of the host.
/// @return The function returns one or more IP addresses of the specified host.
/// @throw Throw an exception if the host is not found.
///
std::vector<std::string> getHostByName(const std::string& sHostname);

///
/// Get host names which which assigned to a specified IP address.
///
/// @param sAddress - IP address of the host.
/// @return The function returns one or more names of the specified host.
/// @throw Throw an exception if the host is not found.
///
std::vector<std::string> getHostByAddress(const std::string& sAddress);

} // namespace net
} // namespace cmd
/// @} 
