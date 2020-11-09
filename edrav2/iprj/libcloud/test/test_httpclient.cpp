//
// edrav2.libcloud.unittest
//
// HTTP client object tests 
//
// Autor: Denis Bogdanov (16.03.2019)
// Reviewer: Yury Ermakov (02.07.2019)
//
#include "pch.h"

using namespace openEdr;
static const char c_sGcpRootUrl[] = "https://api.cwatchedr.com/endpoint";
static const char c_sGcpEndpointId[] = "EndpointIdPlaceholder";
static const char c_sGcpCompanyId[] = "CompanyIdPlaceholder";

//
//
//
TEST_CASE("net.parseUrl")
{	
	auto fnScenario = [](const std::string& sFullPath, 
		std::string_view sScheme, std::string_view sHost, std::string_view sPort, 
		std::string_view sPath, std::string_view sQuery, bool fUseSSL)
	{
		Variant vInfo;
		REQUIRE_NOTHROW(vInfo = net::parseUrl(sFullPath));
		REQUIRE(vInfo.get("scheme", "") == sScheme);
		REQUIRE(vInfo.get("host", "") == sHost);
		REQUIRE(vInfo.get("port", "") == sPort);
		REQUIRE(vInfo.get("path", "") == sPath);
		REQUIRE(vInfo.get("query", "wrong") == sQuery);
		REQUIRE(vInfo.get("useSSL", !fUseSSL) == fUseSSL);
	};

	SECTION("http_default")
	{
		fnScenario("http://test.com/root/path/dir", "http",
			"test.com", "80", "/root/path/dir", "", false);
	}

	SECTION("http_with_port")
	{
		fnScenario("http://test.com:8080/root/path/dir", "http",
			"test.com", "8080", "/root/path/dir", "", false);
	}

	SECTION("http_full_with_slash")
	{
		fnScenario("http://test.com/root/path/dir/", "http",
			"test.com", "80", "/root/path/dir/", "", false);
	}

	SECTION("https_with_port")
	{
		fnScenario("https://test.com:8080/root/path/dir", "https",
			"test.com", "8080", "/root/path/dir", "", true);
	}

	SECTION("https_with_userdata")
	{
		fnScenario("https://user@test.com:8080/root/path/dir", "https",
			"test.com", "8080", "/root/path/dir", "", true);
	}

	SECTION("https_with_params")
	{
		fnScenario("https://test.com:8080/root/path/dir/?var=v1", "https",
			"test.com", "8080", "/root/path/dir/", "?var=v1", true);
	}

	SECTION("https_with_empty_params")
	{
		fnScenario("https://test.com:8080/root/path/dir/?", "https",
			"test.com", "8080", "/root/path/dir/", "", true);
	}

	SECTION("http_no_dir")
	{
		fnScenario("http://test.com:8080", "http",
			"test.com", "8080", "/", "", false);
	}

	SECTION("http_no_dir_with_slash")
	{
		fnScenario("http://test.com:8080/", "http",
			"test.com", "8080", "/", "", false);
	}

	SECTION("http_no_dir_with_params")
	{
		fnScenario("http://test.com:8080/?var=v1&var2=2", "http",
			"test.com", "8080", "/", "?var=v1&var2=2", false);
	}
}

//
//
//
TEST_CASE("net.combineUrl")
{
	auto fnScenario = [](const std::string& sFullPath,
		std::string_view sScheme, std::string_view sHost, 
		std::string_view sPort, std::string_view sPath)
	{
		Dictionary vInfo;
		vInfo.put("scheme", sScheme);
		vInfo.put("host", sHost);
		vInfo.put("port", sPort);
		vInfo.put("path", sPath);
		REQUIRE(sFullPath == net::combineUrl(vInfo));
	};

	SECTION("http_default")
	{
		fnScenario("http://test.com:80/root/path/dir", "http",
			"test.com", "80", "/root/path/dir");
	}

	SECTION("http_full_with_slash")
	{
		fnScenario("http://test.com:80/root/path/dir/", "http",
			"test.com", "80", "/root/path/dir/");
	}

	SECTION("https_with_port")
	{
		fnScenario("https://test.com:8080/root/path/dir", "https",
			"test.com", "8080", "/root/path/dir");
	}

	SECTION("http_no_dir_with_slash")
	{
		fnScenario("http://test.com:8080/", "http",
			"test.com", "8080", "/");
	}

}

//
//
//
TEST_CASE("HttpClient.get_http_stream")
{
	net::HttpClient hc;
	auto v = queryInterface<io::IReadableStream>(
		hc.get("www.mock-server.com", Dictionary({ {"triesCount", 3}, {"tryTimeout", 5000} })) );
	char c_sConstData[] = "<!doctype html>";
	char sData[sizeof(c_sConstData)]; *sData = 0;
	v->read(sData, sizeof(sData));
	REQUIRE(_strnicmp(sData, c_sConstData, sizeof(c_sConstData) - 1) == 0);
}

//
//
//
TEST_CASE("HttpClient.get_unexisted")
{
	net::HttpClient hc;
	REQUIRE_THROWS_AS(hc.get("www.mock-server99999999.com"), error::ConnectionError);
}

//
//
//
TEST_CASE("HttpClient.get_https_stream")
{
	net::HttpClient hc;
	auto v = queryInterface<io::IReadableStream>(
		hc.get("https://www.mock-server.com", Dictionary({ {"triesCount", 3}, {"tryTimeout", 5000} })));
	char c_sConstData[] = "<!doctype html>";
	char sData[sizeof(c_sConstData)]; *sData = 0;
	v->read(sData, sizeof(sData));
	REQUIRE(_strnicmp(sData, c_sConstData, sizeof(c_sConstData) - 1) == 0);
}

//
//
//
TEST_CASE("HttpClient.get_http_json")
{
	net::HttpClient hc("", Dictionary({ {"resultType", "application/json"} }));
	//auto v = hc.get("http://my-json-server.typicode.com/typicode/demo/profile");
	auto v = hc.get("http://echo.jsontest.com/name/value", Dictionary({ {"triesCount", 3}, {"tryTimeout", 5000} }));
	REQUIRE(v.has("name"));
}

//
//
//
TEST_CASE("HttpClient.get_http_rel_json", "[!mayfail]")
{
	net::HttpClient hc("http://my-json-server.typicode.com/typicode/demo/",
		Dictionary({ {"resultType", "application/json"} }));
	auto v = hc.get("profile", Dictionary({ {"triesCount", 3}, {"timeout", 5000} }));
	REQUIRE(v.has("name"));
}

//
//
//
TEST_CASE("HttpClient.get_http_empty_json")
{
	net::HttpClient hc("http://my-json-server.typicode.com/typicode/demo/profile",
		Dictionary({ {"resultType", "application/json"} }));
	auto v = hc.get("", Dictionary({ {"triesCount", 3}, {"tryTimeout", 5000} }));
	REQUIRE(v.has("name"));
}

//
//
//
TEST_CASE("HttpClient.get_https_json")
{
	net::HttpClient hc;
	auto v = hc.get("https://my-json-server.typicode.com/typicode/demo/profile",
		Dictionary({ {"triesCount", 3}, {"timeout", 5000} }));
	REQUIRE(v.has("name"));
}

//
//
//
TEST_CASE("HttpClient.get_http_big_stream")
{
	net::HttpClient hc;
	auto v = queryInterface<io::IReadableStream>(hc.get("http://www.ovh.net/files/10Mio.dat", Dictionary({
		{"connectionTimeout", 5000},
		{"operationTimeout", 60000},
		{"triesCount", 3}, 
		{"tryTimeout", 5000}
	})));
	auto n = v->getSize();
	auto hash = crypt::sha1::getHash(v);
	const char c_sHash[] = "984bc7daae5f509357fb6694277a9852db61f2a7";
	auto sHash = string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
	REQUIRE(sHash == c_sHash);
}

//
//
//
TEST_CASE("HttpClient.get_https_big_stream")
{
	net::HttpClient hc;
	auto v = queryInterface<io::IReadableStream>(
		hc.get("https://mirror.yandex.ru/pub/tools/crosstool/files/bin/x86_64/8.1.0/x86_64-gcc-8.1.0-nolibc-hppa64-linux.tar.xz",
		Dictionary({
			{"connectionTimeout", 5000},
			{"operationTimeout", 60000},
			{"triesCount", 3}, 
			{"timeout", 5000}
		})
	));
	auto hash = crypt::sha1::getHash(v);
	const char c_sHash[] = "b55c2655b4d77cc408254c7124f1c52e41c3198e";
	auto sHash = string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
	REQUIRE(sHash == c_sHash);
}

//
//
//
TEST_CASE("HttpClient.download_http")
{
	net::HttpClient hc;
	auto sTempPath = getTestTempPath() / "download.test";	
	hc.download(sTempPath, "http://www.ovh.net/files/10Mio.dat", Dictionary({
		{"connectionTimeout", 5000},
		{"operationTimeout", 60000},
		{"triesCount", 3}, 
		{"timeout", 5000}
	}));
	auto pFile = io::createFileStream(sTempPath, io::FileMode::Read);	
	auto hash = crypt::sha1::getHash(pFile);
	pFile.reset();
	std::filesystem::remove(sTempPath);
	const char c_sHash[] = "984bc7daae5f509357fb6694277a9852db61f2a7";
	auto sHash = string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
	REQUIRE(sHash == c_sHash);
}


//
//
//
TEST_CASE("HttpClient.post_dictionary")
{
	net::HttpClient hc(c_sGcpRootUrl);
	auto vData = Dictionary({
		{"companyID", c_sGcpCompanyId},
		{"endpointID", c_sGcpEndpointId}
	});
	auto pData = io::createMemoryStream();
	variant::serializeToJson(queryInterface<io::IWritableStream>(pData), vData);
	auto v = hc.post("heartbeat", pData,
		Dictionary({ {"contentType", "application/json"}, {"triesCount", 3}, {"timeout", 5000} }));
}

//
//
//
TEST_CASE("HttpClient.post_stream")
{
	net::HttpClient hc(c_sGcpRootUrl);
	std::string sData("{\"companyID\":\"");
	sData += c_sGcpCompanyId;
	sData += "\",\"endpointID\":\"";
	sData += c_sGcpEndpointId;
	sData += "\"}";
	auto v = hc.post("heartbeat", sData,
		Dictionary({ {"contentType", "application/json"}, {"triesCount", 3}, {"timeout", 5000} }));
}

//
//
//
TEST_CASE("HttpClient.head_http")
{
	net::HttpClient hc;
	// http://www.ovh.net/files/
	auto v = hc.head("http://www.ovh.net/files/10Mio.dat", Dictionary({ {"triesCount", 3}, {"timeout", 5000} }));
	REQUIRE(v.get("contentLength", 0) == 10485760);
}

//
//
//
TEST_CASE("HttpClient.head_https")
{
	net::HttpClient hc;
	auto v = hc.head("https://mirror.yandex.ru/pub/tools/crosstool/files/bin/x86_64/8.1.0/x86_64-gcc-8.1.0-nolibc-hppa64-linux.tar.xz",
		Dictionary({ {"triesCount", 3}, {"timeout", 5000} }));
	REQUIRE(v.get("contentLength", 0) == 12012256);
}

//
//
//
TEST_CASE("HttpClient.cancel_http")
{
	auto fnScenario = []()
	{
		net::HttpClient hc;
		auto result = std::async(std::launch::async,
			[&]()
			{
				try
				{
					(void)hc.get("http://www.ovh.net/files/100Mio.dat", Dictionary({ {"maxResponseSize", 200*1024*1024} }));
					CHECK(false);
				}
				catch (const error::ConnectionError&) {}
				catch (const error::OperationCancelled&) {}
				catch (...)
				{
					CHECK(false);
				}
			});

		result.wait_for(std::chrono::seconds(1));
		hc.abort();
		result.wait();
	};

	SECTION("one")
	{
		REQUIRE_NOTHROW(fnScenario());
	}
}

//
//
//
TEST_CASE("HttpClient.cancel_https")
{
	auto fnScenario = [&]()
	{
		net::HttpClient hc;
		auto result = std::async(std::launch::async,
			[&]()
			{
				try
				{
					auto vData = hc.get("https://mirror.yandex.ru/pub/OpenBSD/6.5/src.tar.gz",
						Dictionary({ {"maxResponseSize", 200 * 1024 * 1024} }));
					//auto pStream = queryInterfaceSafe<io::IReadableStream>(vData);
					//CHECK(pStream != nullptr);
					//Size nSize = pStream->getSize();
					CHECK(false);
				}
				catch (const error::ConnectionError&) {}
				catch (const error::OperationCancelled&) {}
				catch (...)
				{
					CHECK(false);
				}
			});

		result.wait_for(std::chrono::seconds(1));
		hc.abort();
		result.wait();
	};
	
	SECTION("one")
	{
		REQUIRE_NOTHROW(fnScenario());
	}
}
