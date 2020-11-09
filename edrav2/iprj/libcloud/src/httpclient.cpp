//
// edrav2.libcore
// 
// Author: Denis Bogdanov (11.04.2019)
// Reviewer: Yury Ermakov (16.04.2019)
//
///
/// @file Base http client class implementation
/// @addtogroup cloudNetwork Generic network support
/// @{
///
#include "pch.h"
#include <net.hpp>

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "httpclnt"

namespace openEdr {
namespace net {

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

static const Size c_nSendTimeoutMs = 2000;
static const Size c_nReceiveTimeoutMs = 2000;
static const Size c_nBlockSize = 0x20000; // 128 KB
static const char c_sBoundary[] = "--------------maxh----------5dd7dc868165";
static const char c_sBoundaryField[] = "boundary=";
static const char c_sHttpPrefix[] = "http://";
static const char c_sHttpsPrefix[] = "https://";

//
//
//
struct ScopeExit 
{
private:
	std::function<void()> f_;
public:
	ScopeExit(std::function<void()> f) : f_(f) {}
	~ScopeExit() { f_(); }
};

//
//
//
Variant parseUrl(network::uri u)
{
	Dictionary vUrl;
	vUrl.put("scheme", u.has_scheme() ? u.scheme().to_string() : std::string("http"));
	bool isSsl = vUrl["scheme"] == "https";
	vUrl.put("host", u.has_host() ? u.host().to_string() : std::string("localhost"));
	vUrl.put("port", u.has_port() ? u.port().to_string() : (isSsl ? "443" : "80"));
	auto sPath = u.path().to_string();
	vUrl.put("path", sPath.empty() ? std::string("/") : sPath);
	vUrl.put("useSSL", isSsl);
	vUrl.put("query", u.has_query() && !u.query().empty() ? "?" + u.query().to_string() : "");
	return vUrl;
}

//
//
//
std::string constructUrl(const std::string& sUrl)
{
	if (boost::istarts_with(sUrl, c_sHttpsPrefix) ||
		boost::istarts_with(sUrl, c_sHttpPrefix))
		return sUrl;
	return c_sHttpPrefix + sUrl;
}

//
//
//
bool isFullUrl(const std::string& sUrl)
{
	if (boost::istarts_with(sUrl, c_sHttpsPrefix) ||
		boost::istarts_with(sUrl, c_sHttpPrefix))
			return true;
	return false;
}

//
//
//
Variant parseUrl(const std::string& sUrl)
{
	return parseUrl(network::uri(sUrl));
}

//
//
//
std::string combineUrl(Variant vUrl)
{
	std::string sUrl(vUrl.get("scheme", "http"));
	sUrl += "://";
	sUrl += vUrl.get("host", "localhost");
	sUrl += ":";
	sUrl += vUrl.get("port", "80");
	sUrl += vUrl.get("path", "");
	return sUrl;
}

//
//
//
class StreamHolder
{
	ObjPtr<IObject> m_pStream;

public:
	StreamHolder(ObjPtr<io::IReadableStream> pStream) : m_pStream(pStream) {}
	StreamHolder(ObjPtr<io::IWritableStream> pStream) : m_pStream(pStream) {}
	StreamHolder() 
	{
		m_pStream = io::createNullStream(0);
	}
	StreamHolder(const StreamHolder& sh)
	{
		m_pStream = sh.m_pStream;
	}
	StreamHolder(StreamHolder&& sh) noexcept
	{
		m_pStream = sh.m_pStream;
		sh.m_pStream.reset();
	}
	StreamHolder& operator=(const StreamHolder& sh)
	{
		m_pStream = sh.m_pStream;
		return *this;
	}

	template<class I>
	ObjPtr<I> get() const { return queryInterface<I>(m_pStream); }

	template<class I>
	ObjPtr<I> getSafe() const { return queryInterfaceSafe<I>(m_pStream); }

};

//
//
//
struct StreamBody
{
	using value_type = StreamHolder;

	//
	// Getting data size
	//
	static std::uint64_t size(const value_type& v)
	{
		auto pS = v.getSafe<io::ISeekableStream>();
		return pS == nullptr? 0 : pS->getSize();
	}

	//
	// Body-writer concept
	//
	class writer
	{
		ObjPtr<io::IReadableStream> m_pStream;
		Byte m_buffer[c_nBlockSize] = {};

	public:
		using const_buffers_type = asio::const_buffer;

		//
		// Constructor
		//
		template<bool isRequest, class Fields>
		explicit writer(http::header<isRequest, Fields> const&, const value_type& v) : 
			m_pStream(v.get<io::IReadableStream>()) {}

		//
		// Calls for initialization before first read
		//
		void init(beast::error_code& ec)
		{
			ec.assign(0, ec.category());
			try
			{
				m_pStream->setPosition(0);
			}
			catch (error::Exception& e)
			{
				ec.assign((int)e.getErrorCode(), asio::error::misc_category);
			}
		}

		//
		// Calls for read data
		//
		boost::optional<std::pair<const_buffers_type, bool>> get(beast::error_code& ec)
		{
			ec.assign(0, ec.category());
			Size nSize = 0;
			try
			{
				nSize = m_pStream->read(m_buffer, std::size(m_buffer));
			}
			catch (error::Exception& e)
			{
				ec.assign((int)e.getErrorCode(), asio::error::misc_category);
			}

			if (nSize == 0)
				return boost::none;

			return { {const_buffers_type{m_buffer, nSize},
				nSize == std::size(m_buffer)} };
		}
	};

	//
	// Body-reader concept
	//
	class reader
	{
		ObjPtr<io::IWritableStream> m_pStream;

	public:
		
		//
		// Constructor
		//
		template<bool isRequest, class Fields>
		explicit reader(http::header<isRequest, Fields>&, const value_type& v)
			: m_pStream(v.get<io::IWritableStream>()) 
		{
		}

		//
		// Calls for initialization before first write
		//
		void init(boost::optional<std::uint64_t> const& /*length*/, beast::error_code& ec)
		{
			ec.assign(0, ec.category());
			try
			{
				m_pStream->setPosition(0);
			}
			catch (error::Exception& e)
			{
				ec.assign((int)e.getErrorCode(), asio::error::misc_category);
			}
		}

		//
		// Writes data
		//
		template<class ConstBufferSequence>
		std::size_t put(ConstBufferSequence const& buffers, beast::error_code& ec)
		{
			ec.assign(0, ec.category());						
			try
			{
				for (auto b : buffers)
					m_pStream->write(b.data(), b.size());
			}
			catch (error::Exception& e)
			{
				ec.assign((int)e.getErrorCode(), asio::error::misc_category);
			}
			return asio::buffer_size(buffers);
		}

		//
		// Finish writting
		//
		void finish(beast::error_code& ec)
		{
			m_pStream->flush();
			ec.assign(0, ec.category());
		}
	};
};

//
//
//
class HttpClient::Impl : public std::enable_shared_from_this<HttpClient::Impl>
{
private:

	using ResolverResults = tcp::resolver::results_type;
	using ResolverCacheMap = std::unordered_map<std::string, ResolverResults>;
	using StreamBasedResponse = http::response<StreamBody>;
	using StreamBasedRequest = http::request<StreamBody>;
	using ResponseParser = http::response_parser<StreamBasedResponse::body_type>;

	//
	//
	//
	class SessionController
	{
	public:
		virtual void cancel() = 0;
	};

	static constexpr char c_sChromeUA[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.77 Safari/537.36";
	static constexpr Size c_nMaxResponseSize = 20 * 1024 * 1024; // 20MB
	static constexpr char c_sUserAgent[] = "edr.agent/2.0";
	static constexpr Size c_nDelayInterval = 100;

	std::string m_sRootUrl;
	Variant m_vParams;
	asio::io_context m_ioCtx;
	std::atomic_bool m_fCancel = false;
	std::mutex m_mtxSessionController;
	SessionController* m_pCurrSessionController = nullptr;

	//
	//
	//
	class PlainSession : public SessionController
	{
		asio::io_context& m_ioCtx;
		tcp::resolver m_resolver;
		StreamBasedRequest m_request;
		ResponseParser m_responseParser;
		beast::flat_buffer m_buffer;
		beast::tcp_stream m_dataStream;
		std::atomic_bool m_fCancel{ false };
		Size m_nOperationTimeout = 10000; // 10s
		Size m_nConnectTimeout = 5000;
	
	public:

		//
		//
		//
		PlainSession(asio::io_context& ioCtx, StreamBasedRequest& request, StreamBasedResponse& response,
			Size nMaxResponseSize, Size nConnectTimeout, Size nOperationTimeout) :
			m_ioCtx(ioCtx), m_request(request), m_responseParser(std::move(response)),
			m_dataStream(ioCtx), m_resolver(ioCtx), m_nOperationTimeout(nOperationTimeout),
			m_nConnectTimeout(nConnectTimeout)
		{
			if (nMaxResponseSize == 0)
				m_responseParser.skip(true);
			else
				m_responseParser.body_limit(nMaxResponseSize);
			m_responseParser.eager(true);
		}

		//
		//
		//
		StreamBasedResponse run(Variant vAddress)
		{
			m_resolver.async_resolve(vAddress["host"], vAddress.get("port", "80"),
				[this](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type results)
			{
				if (m_fCancel)
					error::OperationCancelled(SL).throwException();
				if (ec)
					error::BoostSystemError(SL, ec, "Failed to resolve the host").throwException();

				// Make the connection on the IP address we get from a lookup
				m_dataStream.expires_after(std::chrono::milliseconds(m_nConnectTimeout));
				m_dataStream.async_connect(results, [this](const boost::system::error_code& ec,
					const boost::asio::ip::tcp::endpoint& endpoint)
				{
					boost::ignore_unused(endpoint);

					if (m_fCancel)
						error::OperationCancelled(SL).throwException();
					if (ec)
						error::BoostSystemError(SL, ec, "Failed to connect to the host").throwException();

					// Send the HTTP request to the remote host
					m_dataStream.expires_after(std::chrono::milliseconds(m_nOperationTimeout));
					http::async_write(m_dataStream, m_request,
						[this](const boost::system::error_code& ec, std::size_t bytes_transferred)
					{
						boost::ignore_unused(bytes_transferred);

						if (m_fCancel)
							error::OperationCancelled(SL).throwException();
						if (ec)
							error::BoostSystemError(SL, ec, "Failed to write the stream").throwException();

						// Receive the HTTP response
						m_dataStream.expires_after(std::chrono::milliseconds(m_nOperationTimeout));
						http::async_read(m_dataStream, m_buffer, m_responseParser,
							[this](const boost::system::error_code& ec, std::size_t bytes_transferred)
						{
							boost::ignore_unused(bytes_transferred);

							if (m_fCancel)
								error::OperationCancelled(SL).throwException();
							if (ec)
								error::BoostSystemError(SL, ec, "Failed to write the stream").throwException();

							boost::system::error_code ecClose;
							m_dataStream.socket().shutdown(tcp::socket::shutdown_both, ecClose);
							if (ecClose && (ecClose != beast::errc::not_connected))
								error::BoostSystemError(SL, ecClose, "Failed to close a session").throwException();
						});
					});
				});
			});

			//	beast::bind_front_handler(&PlainSession::onResolved, shared_from_this()));
			// Run the I/O service. The call will return when the operation is complete.
			m_ioCtx.run();
			return m_responseParser.release();
		}

		//
		//
		//
		void cancel() override
		{
			m_fCancel = true;
			m_dataStream.cancel();
		}
	};

	//
	//
	//
	class SslSession : public SessionController
	{
		asio::io_context& m_ioCtx;
		ssl::context m_sslCtx{ ssl::context::sslv23_client };
		tcp::resolver m_resolver;
		StreamBasedRequest m_request;
		ResponseParser m_responseParser;
		beast::flat_buffer m_buffer;
		beast::ssl_stream<beast::tcp_stream> m_sslStream;
		std::atomic_bool m_fCancel{ false };
		Size m_nOperationTimeout = 10000; // 10s
		Size m_nConnectTimeout = 5000;

	public:

		//
		//
		//
		SslSession(asio::io_context& ioCtx, StreamBasedRequest& request, StreamBasedResponse& response,
			Size nMaxResponseSize, Size nConnectTimeout, Size nOperationTimeout) :
			m_ioCtx(ioCtx), m_request(request), m_responseParser(std::move(response)),
			m_sslStream(ioCtx, m_sslCtx), m_resolver(ioCtx), m_nOperationTimeout(nOperationTimeout),
			m_nConnectTimeout(nConnectTimeout)
		{
			if (nMaxResponseSize == 0)
				m_responseParser.skip(true);
			else
				m_responseParser.body_limit(nMaxResponseSize);
			m_responseParser.eager(true);
		}

		//
		//
		//
		StreamBasedResponse run(Variant vAddress)
		{
			// Set SNI Hostname (many hosts need this to handshake successfully)
			std::string sHost = vAddress["host"];
			if (!SSL_set_tlsext_host_name(m_sslStream.native_handle(), sHost.data()))
				error::ConnectionError(SL, FMT("SSL initialization error <" <<
					::ERR_get_error() << ">")).throwException();

			m_resolver.async_resolve(vAddress["host"], vAddress.get("port", "443"),
				[this](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type results)
			{
				if (m_fCancel)
					error::OperationCancelled(SL).throwException();
				if (ec)
					error::BoostSystemError(SL, ec, "Failed to resolve the host").throwException();

				// Make the connection on the IP address we get from a lookup
				get_lowest_layer(m_sslStream).expires_after(std::chrono::milliseconds(m_nConnectTimeout));
				get_lowest_layer(m_sslStream).async_connect(results, [this](const boost::system::error_code& ec,
					const boost::asio::ip::tcp::endpoint& endpoint)
				{
					boost::ignore_unused(endpoint);

					if (m_fCancel)
						error::OperationCancelled(SL).throwException();
					if (ec)
						error::BoostSystemError(SL, ec, "Failed to connect to the host").throwException();

					// Send the HTTP request to the remote host
					get_lowest_layer(m_sslStream).expires_after(std::chrono::milliseconds(m_nOperationTimeout));
					m_sslStream.async_handshake(ssl::stream_base::client, 
						[this](const boost::system::error_code& ec)
					{
						if (m_fCancel)
							error::OperationCancelled(SL).throwException();
						if (ec)
							error::BoostSystemError(SL, ec, "Failed to establish SSL session").throwException();

						// Send the HTTP request to the remote host
						get_lowest_layer(m_sslStream).expires_after(std::chrono::milliseconds(m_nOperationTimeout));
						http::async_write(m_sslStream, m_request,
							[this](const boost::system::error_code& ec, std::size_t bytes_transferred)
						{
							boost::ignore_unused(bytes_transferred);

							if (m_fCancel)
								error::OperationCancelled(SL).throwException();
							if (ec)
								error::BoostSystemError(SL, ec, "Failed to write the stream").throwException();

							// Receive the HTTP response
							get_lowest_layer(m_sslStream).expires_after(std::chrono::milliseconds(m_nOperationTimeout));
							http::async_read(m_sslStream, m_buffer, m_responseParser,
								[this](const boost::system::error_code& ec, std::size_t bytes_transferred)
							{
								boost::ignore_unused(bytes_transferred);

								if (m_fCancel)
									error::OperationCancelled(SL).throwException();
								if (ec)
									error::BoostSystemError(SL, ec, "Failed to write the stream").throwException();

								get_lowest_layer(m_sslStream).expires_after(std::chrono::milliseconds(m_nOperationTimeout));
								m_sslStream.async_shutdown(
									[this](const boost::system::error_code& ec) 
								{
									// Rationale:
									// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
									// not_connected happens sometimes so don't bother reporting it.
									if (ec && (ec != asio::error::eof) && (ec != ssl::error::stream_truncated))
										error::BoostSystemError(SL, ec, "Failed to close a session").throwException();
								});
							});
						});
					});
				});
			});

			//	beast::bind_front_handler(&PlainSession::onResolved, shared_from_this()));
			// Run the I/O service. The call will return when the operation is complete.
			m_ioCtx.run();
			return m_responseParser.release();
		}

		//
		//
		//
		void cancel() override
		{
			m_fCancel = true;
			get_lowest_layer(m_sslStream).cancel();
		}
	};

public:

	//
	//
	//
	StreamBasedResponse process(Variant vAddress, Variant vParams, StreamBasedRequest& request,
		ObjPtr<io::IWritableStream> pOutStream)
	{
		if (m_fCancel)
			error::OperationCancelled(SL).throwException();

		auto pLock = shared_from_this();

		LOGLVL(Debug, "Send HTTP request to <" << vAddress["host"] << ">");
		if (request.payload_size().has_value() && request.payload_size().get() < 0x1000)
			LOGLVL(Trace, "Request dump:\n" << request << "\n");

		// auto dreq = FMT("Req dump: " << request);

		bool fCheckShutdown = m_vParams.get("checkShutdown", true);
		Size nTriesCount = vParams.get("triesCount", m_vParams.get("triesCount", 1));
		Size nTryTimeout = vParams.get("tryTimeout", m_vParams.get("timeout", c_nDelayInterval * 10));
		Size nOperationTimeout = vParams.get("operationTimeout", m_vParams.get("operationTimeout", 30000));
		Size nConnectionTimeout = vParams.get("connectionTimeout", m_vParams.get("connectionTimeout", 10000));
		Size nMaxResponseSize = vParams.get("maxResponseSize", m_vParams.get("maxResponseSize", c_nMaxResponseSize));

		while (nTriesCount--)
		{
			CMD_TRY
			{
				StreamBasedResponse response;
				if (pOutStream != nullptr)
					response.body() = pOutStream;
				else
					nMaxResponseSize = 0;
				m_ioCtx.restart();

				if (vAddress.get("useSSL", false))
				{
					SslSession s(m_ioCtx, request, response, nMaxResponseSize,
						nConnectionTimeout, nOperationTimeout);
					{
						std::scoped_lock lock(m_mtxSessionController);
						m_pCurrSessionController = &s;
					}
					ScopeExit e([this]() 
					{
						std::scoped_lock lock(m_mtxSessionController);
						m_pCurrSessionController = nullptr;
					});
					response = s.run(vAddress);
				}
				else
				{
					PlainSession s(m_ioCtx, request, response, nMaxResponseSize,
						nConnectionTimeout, nOperationTimeout);
					{
						std::scoped_lock lock(m_mtxSessionController);
						m_pCurrSessionController = &s;
					}
					ScopeExit e([this]()
					{
						std::scoped_lock lock(m_mtxSessionController);
						m_pCurrSessionController = nullptr;
					});
					response = s.run(vAddress);
				}

				if (m_fCancel)
					error::OperationCancelled(SL).throwException();

				if (nTriesCount > 0 && response.result_int() >= 500 && response.result_int() < 600)
				{
					LOGLVL(Detailed, "Server returned code <" << response.result_int() << ">");
					error::ConnectionError(SL, FMT("Data is temporary unavailable (code <" <<
						response.result_int() << ">)")).throwException();
				}

				if (!response.payload_size().has_value())
					LOGLVL(Trace, "Response is empty\n");
				else if(response.payload_size().get() > 0x8000)
					LOGLVL(Trace, "Response size is " << response.payload_size().get() << " bytes\n");
				else
					LOGLVL(Trace, "Response dump:\n" << response << "\n");
				// auto dres = FMT("Req dump: " << response);
				return response;
			}
			CMD_PREPARE_CATCH
			catch (error::ConnectionError&)
			{
				if (m_fCancel)
					error::OperationCancelled(SL).throwException();

				if (nTriesCount == 0)
					throw;

				LOGLVL(Detailed, "Can't connect to the server <" << vAddress["host"] << ">. Retrying...");

				// Reset data stream
				pOutStream->setSize(0);

				// Wait for timeout but check application termination
				Size nDelayCount = nTryTimeout / c_nDelayInterval;
				if (nTryTimeout < c_nDelayInterval)
					std::this_thread::sleep_for(std::chrono::milliseconds(nTryTimeout));
				else while (nDelayCount--)
				{
					if (fCheckShutdown)
					{
						std::string sStage = getCatalogData("app.stage", "");
						if (sStage == "finishing" || sStage == "finished")
							throw;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(c_nDelayInterval));
				}
			}			
		}

		// Normally this line isn't executed
		return StreamBasedResponse(boost::beast::http::status::network_connect_timeout_error, 11);
	}

	//
	//
	//
	std::string getBoundary(std::string_view sValue)
	{
		if (sValue.empty())
			return {};
		Size nBeginPos = sValue.find(c_sBoundaryField);
		if (nBeginPos == sValue.npos)
			return {};
		nBeginPos += sizeof(c_sBoundaryField);
		Size nEndPos = sValue.find(";", nBeginPos);
		return std::string(((nEndPos != sValue.npos) ? 
			sValue.substr(nBeginPos, nEndPos - nBeginPos) : sValue.substr(nBeginPos)));
	}

	//
	//
	//
	void putBoundary(std::string& sDst, std::string_view sBoundary)
	{
		if (sDst.empty())
		{
			sDst += c_sBoundaryField;
			sDst += sBoundary;
		}
		else if (sDst.find(c_sBoundaryField) == sDst.npos)
		{
			sDst += "; ";
			sDst += c_sBoundaryField;
			sDst += sBoundary;
		}
	}

	//
	//
	//
	ObjPtr<io::IReadableStream> createMultipartFormData(Variant vParams, const std::string& sBoundary)
	{
		auto pData = queryInterface<io::IWritableStream>(io::createMemoryStream());
		std::string sBuffer;

		// Write an opening boundary if needed
		if (vParams.isEmpty())
		{
			sBuffer = "\r\n--";
			sBuffer += sBoundary;
		}
		sBuffer += "\r\n";
		io::write(pData, sBuffer);

		for (const auto& [sKey, vValue] : Dictionary(vParams))
		{
			// Write Content-Disposition
			sBuffer = "--";
			sBuffer += sBoundary;
			sBuffer += "\r\nContent-Disposition: form-data; name=\"";
			sBuffer += sKey;
			sBuffer += "\"";
			io::write(pData, sBuffer);

			if (vValue.isDictionaryLike() && vValue.has("stream"))
			{
				auto pStream = queryInterfaceSafe<io::IReadableStream>(vValue["stream"]);
				if (pStream)
				{
					sBuffer = "; filename=\"";
					sBuffer += vValue.get("filename", "file.exe");
					sBuffer += "\"\r\nContent-Type: application/octet-stream\r\n\r\n";
					io::write(pData, sBuffer);
					io::write(pData, pStream, io::c_nMaxIoSize, nullptr, c_nBlockSize);
					io::write(pData, "\r\n");
					continue;
				}
			}
			if (vValue.getType() == variant::ValueType::Object)
			{
				auto pStream = queryInterfaceSafe<io::IReadableStream>(vValue);
				if (pStream)
				{
					io::write(pData, "; filename=\"file.exe\"\r\nContent-Type: application/octet-stream\r\n\r\n");
					io::write(pData, pStream, io::c_nMaxIoSize, nullptr, c_nBlockSize);
					io::write(pData, "\r\n");
					continue;
				}
			}
			if (vValue.getType() == variant::ValueType::String)
			{
				sBuffer = "\r\n\r\n";
				sBuffer += std::string(vValue);
				sBuffer += "\r\n";
				io::write(pData, sBuffer);
			}
			else
			{
				io::write(pData, "\r\n\r\n");
				variant::serializeToJson(pData, vValue);
				io::write(pData, "\r\n");
			}
		}
		
		// Write a closing boundary
		sBuffer = "--";
		sBuffer += sBoundary;
		sBuffer += "--\r\n";
		io::write(pData, sBuffer);
		return queryInterface<io::IReadableStream>(pData);
	}

public:

	Impl(const std::string& sAddress, Variant vParams = Dictionary()) :
		m_sRootUrl(constructUrl(sAddress))
	{
		setParams(vParams);
	}

	Impl(Variant vAddress, Variant vParams = Dictionary()) :
		m_sRootUrl(combineUrl(vAddress))
	{
		setParams(vParams);
	}

	//
	//
	//
	void setRootUrl(const std::string& sAddress)
	{
		m_sRootUrl = constructUrl(sAddress);
	}

	//
	//
	//
	void setParams(Variant vParams)
	{
		m_vParams = vParams;
	}

	//
	//
	//
	Variant get(const std::string& sResource, Variant vParams, 
		ObjPtr<io::IWritableStream> pOutStream = nullptr, Size nDeep = 0)
	{
		if (m_fCancel)
			error::OperationCancelled(SL).throwException();

		// Check max redirections
		if (nDeep > 10)
			error::ConnectionError(SL, "Limit of redirections exceeded").throwException();

		Variant vAddress = isFullUrl(sResource) ? parseUrl(sResource) :
			parseUrl(m_sRootUrl + (m_sRootUrl.back() == '/'? "" : "/") + sResource);
	
		// Set up an GET request
		StreamBasedRequest request { http::verb::get, std::string(vAddress.get("path", "/")) +
			std::string(vAddress.get("query", "")), 11 };
		request.set(http::field::host, std::string(vAddress["host"]));
		Dictionary vHttpHeader = vParams.get("httpHeader", Dictionary());
		for (auto& param : vHttpHeader)
			request.set(std::string(param.first), std::string_view(param.second));
		if (!vHttpHeader.has("userAgent"))
			request.set(http::field::user_agent, m_vParams.get("userAgent", c_sUserAgent));
		auto response = process(vAddress, vParams, request,
			(pOutStream == nullptr) ? queryInterface<io::IWritableStream>(io::createMemoryStream()) : pOutStream);

		bool isJson = boost::istarts_with(response["Content-type"], "application/json") ||
					(m_vParams.get("resultType", "") == "application/json");
		auto pResData = response.body().get<io::IReadableStream>();
		pResData->setPosition(0);

		Variant vResData = (isJson && pResData->getSize() != 0) ?
			variant::deserializeFromJson(pResData) : Variant();

		// Check "301 - Moved Permanently"
		if (response.result_int() == 301)
		{
			LOGLVL(Debug, "Received redirection to <" << response["Location"].to_string() << ">");
			return get(response["Location"].to_string(), vParams, pOutStream, ++nDeep);
		}
		else if (response.result_int() != 200)
		{
			LOGLVL(Critical, "Invalid server response: \n" << response << "\n");
			LOGLVL(Critical, "For request: \n" << request << "\n");
			error::ConnectionError(SL, FMT("Invalid HTTP/GET response <" <<
				response.result_int() << ">, message <" << vResData.get("message", "") << ">")).throwException();
		}

		if (isJson)
			return vResData;

		pResData->setPosition(0);
		return pResData;
	}
		
	//
	//
	//
	Variant head(const std::string& sResource, Variant vParams, Size nDeep = 0)
	{
		if (m_fCancel)
			error::OperationCancelled(SL).throwException();

		// Check max redirections
		if (nDeep > 10)
			error::ConnectionError(SL, "Limit of redirections exceeded").throwException();

		Variant vAddress = isFullUrl(sResource) ? parseUrl(sResource) :
			parseUrl(m_sRootUrl + (m_sRootUrl.back() == '/' ? "" : "/") + sResource);

		// Set up HEAD request
		StreamBasedRequest request{ http::verb::head, std::string(vAddress.get("path", "/")) +
			std::string(vAddress.get("query", "")), 11 };
		request.set(http::field::host, std::string(vAddress["host"]));
		Dictionary vHttpHeader = vParams.get("httpHeader", Dictionary());
		for (auto& param : vHttpHeader)
			request.set(std::string(param.first), std::string_view(param.second));
		if (!vHttpHeader.has("userAgent"))
			request.set(http::field::user_agent, m_vParams.get("userAgent", c_sUserAgent));
		
		auto response = process(vAddress, vParams, request, nullptr);

		// Check "301 - Moved Permanently"
		if (response.result_int() == 301)
		{
			LOGLVL(Debug, "Received HEAD redirection to <" << response["Location"].to_string() << ">");
			return head(response["Location"].to_string(), vParams, ++nDeep);
		}
		else if (response.result_int() != 200)
		{
			LOGLVL(Critical, "Invalid server response: \n" << response << "\n");
			LOGLVL(Critical, "For request: \n" << request << "\n");
			error::ConnectionError(SL, FMT("Invalid HTTP/HEAD response <" <<
				response.result_int() << ">")).throwException();
		}

		Dictionary vResData;
		if (!response[http::field::content_length].empty())
			vResData.put("contentLength", std::stol(response[http::field::content_length].to_string()));
		if (!response[http::field::content_type].empty())
			vResData.put("contentType", response[http::field::content_type].to_string());

		return vResData;
	}

	//
	//
	//
	Variant post(const std::string& sResource, Variant vData, Variant vParams, Size nDeep = 0)
	{
		if (m_fCancel)
			error::OperationCancelled(SL).throwException();

		// Check max redirections
		if (nDeep > 10)
			error::ConnectionError(SL, "Limit of redirection exceeded").throwException();

		Variant vAddress = isFullUrl(sResource) ? parseUrl(sResource) :
			parseUrl(m_sRootUrl + (m_sRootUrl.back() == '/' ? "" : "/") + sResource);

		Dictionary vHttpHeader = vParams.get("httpHeader", Dictionary());
		std::string sContentType = vHttpHeader.get("Content-Type", vParams.get("contentType",
			(vData.isDictionaryLike() || vData.isSequenceLike()) ? "application/json" : "application/octet-stream"));

		ObjPtr<io::IReadableStream> pData;
		if (vData.isDictionaryLike() && (boost::istarts_with(sContentType, "multipart/form-data")))
		{
			std::string sBoundary = getBoundary(sContentType);
			if (sBoundary.empty())
			{
				putBoundary(sContentType, c_sBoundary);
				vParams.put("contentType", sContentType);
			}
			pData = createMultipartFormData(vData, (sBoundary.empty() ? c_sBoundary : sBoundary));
		}
		else if (vData.getType() == variant::ValueType::Object)
		{
			pData = queryInterface<io::IReadableStream>(vData);
		}
		else if (vData.getType() == variant::ValueType::String)
		{
			pData = io::createMemoryStream();
			io::write(queryInterface<io::IWritableStream>(pData), std::string(vData));
		}
		else
		{
			pData = io::createMemoryStream();
			variant::serializeToJson(queryInterface<io::IRawWritableStream>(pData), vData);
		}

		// Set up an POST request
		StreamBasedRequest request { http::verb::post, std::string(vAddress.get("path", "/")), 11 };
		request.set(http::field::host, std::string(vAddress["host"]));
		for (auto& param : vHttpHeader)
			request.set(std::string(param.first), std::string_view(param.second));
		if (!vHttpHeader.has("userAgent"))
			request.set(http::field::user_agent, m_vParams.get("userAgent", c_sUserAgent));

		request.set(http::field::content_type, sContentType);
		
		request.body() = pData;
		request.prepare_payload();

		auto response = process(vAddress, vParams, request, queryInterface<io::IWritableStream>(io::createMemoryStream()));

		bool isJson = boost::istarts_with(response["Content-type"], "application/json");
		auto pResData = response.body().get<io::IReadableStream>();
		pResData->setPosition(0);

		Variant vResData = (isJson && pResData->getSize() != 0)? 
			variant::deserializeFromJson(pResData) : Variant();

		// Check "301 - Moved Permanently"
		if (response.result_int() == 301)
		{
			// Specify right content type for redirected request
			Variant vNewParams = vParams;
			if (!vParams.has("contentType"))
			{
				vNewParams = vParams.clone();
				vNewParams.put("contentType", sContentType);
			}
			LOGLVL(Debug, "Received POST redirection to <" << response["Location"].to_string() << ">");
			return post(response["Location"].to_string(), pData, vNewParams, ++nDeep);
		}
		else if (response.result_int() != 200)
		{
			LOGLVL(Critical, "Invalid server response: \n" << response << "\n");
			if (sContentType == "application/json")
				LOGLVL(Critical, "For request: \n" << request << "\n");
			error::ConnectionError(SL, FMT("Invalid HTTP/POST response <" <<
				response.result_int() << ">, message <" << vResData.get("message", "") << ">")).throwException();
		}
		
		return isJson? vResData : Variant(pResData);
	}

	//
	//
	//
	void abort()
	{
		LOGLVL(Debug, "Current operation is being aborted");
		{
			std::scoped_lock lock(m_mtxSessionController);
			m_fCancel = true;
			if (m_pCurrSessionController)
				m_pCurrSessionController->cancel();
		}
		m_ioCtx.stop();
	}

	//
	//
	//
	void restart()
	{
		m_fCancel = false;
		LOGLVL(Debug, "Object is restarted");
	}
};

///////////////////////////////////////////////////////////////////////
//
// HttpClient wrapper
//
///////////////////////////////////////////////////////////////////////

//
// We need this definition for right linking
//
HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;
HttpClient::~HttpClient() = default;

//
//
//
HttpClient::HttpClient(const std::string& sAddress, Variant vParams)
{
	m_pImpl = std::make_shared<HttpClient::Impl>(sAddress, vParams);
}

//
//
//
HttpClient::HttpClient(HttpClient::CallTag, Variant vAddress, Variant vParams)
{
	m_pImpl = std::make_shared<HttpClient::Impl>(vAddress, vParams);
}

//
//
//
void HttpClient::checkImplIsValid(SourceLocation sl)
{
	if (!m_pImpl)
		error::InvalidUsage(sl, "Access to empty HttpClient").throwException();
}

//
//
//
Variant HttpClient::get(const std::string& sResource, Variant vParams)
{
	checkImplIsValid(SL);
	return m_pImpl->get(sResource, vParams);
}

//
//
//
Variant HttpClient::get(const std::string& sResource, ObjPtr<io::IWritableStream> pOutStream, Variant vParams)
{
	checkImplIsValid(SL);
	return m_pImpl->get(sResource, vParams, pOutStream);
}

//
//
//
Variant HttpClient::head(const std::string& sResource, Variant vParams)
{
	checkImplIsValid(SL);
	return m_pImpl->head(sResource, vParams);
}

//
//
//
void HttpClient::download(std::filesystem::path filePath, const std::string& sResource, Variant vParams)
{
	checkImplIsValid(SL);
	auto pStream = queryInterface<io::IWritableStream>(io::createFileStream(filePath,
		io::FileMode::Write | io::FileMode::CreatePath | io::FileMode::Truncate));
	(void)m_pImpl->get(sResource, vParams, pStream);
}

//
//
//
Variant HttpClient::post(const std::string& sResource, 
	Variant vData, Variant vParams)
{
	checkImplIsValid(SL);
	return m_pImpl->post(sResource, vData, vParams);
}

//
//
//
Variant HttpClient::post(Variant vData, Variant vParams)
{
	checkImplIsValid(SL);
	return m_pImpl->post("", vData, vParams);
}

//
//
//
void HttpClient::setRootUrl(const std::string& sAddress)
{
	checkImplIsValid(SL);
	m_pImpl->setRootUrl(sAddress);
}

//
//
//
void HttpClient::setParams(Variant vParams)
{
	checkImplIsValid(SL);
	m_pImpl->setParams(vParams);
}

//
//
//
void HttpClient::abort()
{
	checkImplIsValid(SL);
	m_pImpl->abort();
}

//
//
//
void HttpClient::restart()
{
	checkImplIsValid(SL);
	m_pImpl->restart();
}

} // namespace net
} // namespace openEdr 
/// @}
