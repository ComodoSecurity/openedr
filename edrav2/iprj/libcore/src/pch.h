#ifndef PCH_H
#define PCH_H

#ifndef NOMINMAX
# define NOMINMAX
#endif

#include <extheaders.h>

#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <atomic>
#include <random>
#include <exception>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <boost/chrono.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast.hpp>

// jsonrpccpp library
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/tcpsocketclient.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/httpserver.h>

// crashpad library
#include <client/crashpad_client.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/simulate_crash.h>

// utf8 library
#include <utf8.h>

// nlohmann library
#include <nlohmann/json.hpp>

// jsoncpp library
#include <json/json.h>

// Common EDR Agent external raw API files
#include <cmd/sys.hpp>
#include <cmd/sys/net.hpp>

// Internal headers for increasing of compilation speed 
#include "basic.hpp"
#include "error.hpp"
#include "object.hpp"
#include "variant.hpp"
#include "logging.hpp"
#include "core.hpp"
//#include "common.hpp"

#endif //PCH_H
