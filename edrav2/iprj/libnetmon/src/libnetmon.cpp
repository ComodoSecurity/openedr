//
// edrav2.libnetmon project
//
// Reviewer: Denis Bogdanov (02.09.2019)
//
// Library classes registration
//
#include "pch_win.h"
#include "controller.h"
#include "nfwrapper_win.h"
#include "parser_http_win.h"
#include "parser_ftp_win.h"
#include "parser_dns.h"

//
// Classes registration
//
CMD_BEGIN_LIBRARY_DEFINITION(libnetmon)
CMD_DEFINE_LIBRARY_CLASS(netmon::win::NetFilterWrapper)
CMD_DEFINE_LIBRARY_CLASS(netmon::NetworkMonitorController)
CMD_DEFINE_LIBRARY_CLASS(netmon::win::HttpParserFactory)
CMD_DEFINE_LIBRARY_CLASS(netmon::win::FtpParserFactory)
CMD_DEFINE_LIBRARY_CLASS(netmon::DnsUdpParserFactory)
CMD_END_LIBRARY_DEFINITION(libnetmon)

