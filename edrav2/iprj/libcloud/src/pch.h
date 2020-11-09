//
// edrav2.libcloud project
//
// Author: Yury Ermakov (11.03.2019)
// Reviewer: Denis Bogdanov (15.03.2019)
//
#pragma once

#define OS_DEPENDENT_PCH
#include "extheaders.h"
#include <libsyswin/inc/libsyswin.h>
#include <unordered_set>
#include <chrono>
#include <numeric>
#include <vector>

// FIXME: Exclude AWS support
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>
#include <aws/firehose/FirehoseClient.h>
#include <aws/firehose/model/PutRecordBatchRequest.h>

// Google Cloud PubSub
#pragma warning(push)
#pragma warning(disable: 4100 4125 4127 4244 4267)
#include <grpcpp/grpcpp.h>
#include <google/pubsub/v1/pubsub.grpc.pb.h>
#pragma warning(pop)

// see: https://github.com/chriskohlhoff/asio/issues/290#issuecomment-371867040
// #define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING

#ifdef PLATFORM_WIN
#pragma warning(push)
#pragma warning(disable:4702)
#endif
#define BOOST_ASIO_NO_WIN32_LEAN_AND_MEAN
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#ifdef PLATFORM_WIN
#pragma warning(pop)
#endif

#ifdef PLATFORM_WIN
#pragma warning(push)
#pragma warning(disable:4244)
#endif
#include <network/uri/uri.hpp>
#ifdef PLATFORM_WIN
#pragma warning(pop)
#endif

// Common EDR Agent external raw API files
#include <cmd/sys.hpp>
#include <cmd/sys/net.hpp>
