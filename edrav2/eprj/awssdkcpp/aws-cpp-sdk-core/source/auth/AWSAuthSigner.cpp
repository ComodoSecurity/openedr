/*
  * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License").
  * You may not use this file except in compliance with the License.
  * A copy of the License is located at
  *
  *  http://aws.amazon.com/apache2.0
  *
  * or in the "license" file accompanying this file. This file is distributed
  * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
  * express or implied. See the License for the specific language governing
  * permissions and limitations under the License.
  */

#include <aws/core/auth/AWSAuthSigner.h>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/crypto/Sha256.h>
#include <aws/core/utils/crypto/Sha256HMAC.h>

#include <cstdio>
#include <iomanip>
#include <math.h>
#include <string.h>

using namespace Aws;
using namespace Aws::Client;
using namespace Aws::Auth;
using namespace Aws::Http;
using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static const char* EQ = "=";
static const char* SIGNATURE = "Signature";
static const char* AWS_HMAC_SHA256 = "AWS4-HMAC-SHA256";
static const char* AWS4_REQUEST = "aws4_request";
static const char* SIGNED_HEADERS = "SignedHeaders";
static const char* CREDENTIAL = "Credential";
static const char* NEWLINE = "\n";
static const char* X_AMZ_SIGNED_HEADERS = "X-Amz-SignedHeaders";
static const char* X_AMZ_ALGORITHM = "X-Amz-Algorithm";
static const char* X_AMZ_CREDENTIAL = "X-Amz-Credential";
static const char* UNSIGNED_PAYLOAD = "UNSIGNED-PAYLOAD";
static const char* X_AMZ_SIGNATURE = "X-Amz-Signature";
static const char* SIGNING_KEY = "AWS4";
static const char* LONG_DATE_FORMAT_STR = "%Y%m%dT%H%M%SZ";
static const char* SIMPLE_DATE_FORMAT_STR = "%Y%m%d";
static const char* EMPTY_STRING_SHA256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

static const char* v4LogTag = "AWSAuthV4Signer";

namespace Aws
{
    namespace Auth
    {
        const char SIGV4_SIGNER[] = "SignatureV4";
        const char NULL_SIGNER[] = "NullSigner";
    }
}

static Aws::String CanonicalizeRequestSigningString(HttpRequest& request, bool urlEscapePath)
{
    request.CanonicalizeRequest();
    Aws::StringStream signingStringStream;
    signingStringStream << HttpMethodMapper::GetNameForHttpMethod(request.GetMethod());

    URI uriCpy = request.GetUri();
    // Many AWS services do not decode the URL before calculating SignatureV4 on their end.
    // This results in the signature getting calculated with a double encoded URL.
    // That means we have to double encode it here for the signature to match on the service side.
    if(urlEscapePath)
    {
        // RFC3986 is how we encode the URL before sending it on the wire.
        auto rfc3986EncodedPath = URI::URLEncodePathRFC3986(uriCpy.GetPath());
        uriCpy.SetPath(rfc3986EncodedPath);
        // However, SignatureV4 uses this URL encoding scheme
        signingStringStream << NEWLINE << uriCpy.GetURLEncodedPath() << NEWLINE;
    }
    else
    {
        // For the services that DO decode the URL first; we don't need to double encode it.
        uriCpy.SetPath(uriCpy.GetURLEncodedPath());
        signingStringStream << NEWLINE << uriCpy.GetPath() << NEWLINE;
    }

    if (request.GetQueryString().find('=') != std::string::npos)
    {
        signingStringStream << request.GetQueryString().substr(1) << NEWLINE;
    }
    else if (request.GetQueryString().size() > 1)
    {
        signingStringStream << request.GetQueryString().substr(1) << "=" << NEWLINE;
    }
    else
    {
        signingStringStream << NEWLINE;
    }

    return signingStringStream.str();
}

static Http::HeaderValueCollection CanonicalizeHeaders(Http::HeaderValueCollection&& headers)
{
    Http::HeaderValueCollection canonicalHeaders;
    for (const auto& header : headers)
    {
        auto trimmedHeaderName = StringUtils::Trim(header.first.c_str());
        auto trimmedHeaderValue = StringUtils::Trim(header.second.c_str());

        //multiline gets converted to line1,line2,etc...
        auto headerMultiLine = StringUtils::SplitOnLine(trimmedHeaderValue);
        Aws::String headerValue = headerMultiLine.size() == 0 ? "" : headerMultiLine[0];

        if (headerMultiLine.size() > 1)
        {
            for(size_t i = 1; i < headerMultiLine.size(); ++i)
            {
                headerValue += ",";
                headerValue += StringUtils::Trim(headerMultiLine[i].c_str());
            }
        }

        //duplicate spaces need to be converted to one.
        Aws::String::iterator new_end =
            std::unique(headerValue.begin(), headerValue.end(),
                [=](char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }
        );
        headerValue.erase(new_end, headerValue.end());

        canonicalHeaders[trimmedHeaderName] = headerValue;       
    }

    return canonicalHeaders;
}

AWSAuthV4Signer::AWSAuthV4Signer(const std::shared_ptr<Auth::AWSCredentialsProvider>& credentialsProvider,
    const char* serviceName, const Aws::String& region, PayloadSigningPolicy signingPolicy, bool urlEscapePath) :
    m_includeSha256HashHeader(true),
    m_credentialsProvider(credentialsProvider),
    m_serviceName(serviceName),
    m_region(region),
    m_hash(Aws::MakeUnique<Aws::Utils::Crypto::Sha256>(v4LogTag)),
    m_HMAC(Aws::MakeUnique<Aws::Utils::Crypto::Sha256HMAC>(v4LogTag)),
    m_unsignedHeaders({"user-agent", "x-amzn-trace-id"}),
    m_payloadSigningPolicy(signingPolicy),
    m_urlEscapePath(urlEscapePath)
{
    //go ahead and warm up the signing cache.
    ComputeHash(credentialsProvider->GetAWSCredentials().GetAWSSecretKey(), DateTime::CalculateGmtTimestampAsString(SIMPLE_DATE_FORMAT_STR));
}

AWSAuthV4Signer::~AWSAuthV4Signer()
{
    // empty destructor in .cpp file to keep from needing the implementation of (AWSCredentialsProvider, Sha256, Sha256HMAC) in the header file 
}


bool AWSAuthV4Signer::ShouldSignHeader(const Aws::String& header) const
{
    return m_unsignedHeaders.find(Aws::Utils::StringUtils::ToLower(header.c_str())) == m_unsignedHeaders.cend();
}

bool AWSAuthV4Signer::SignRequest(Aws::Http::HttpRequest& request) const
{
    return SignRequest(request, true/*signBody*/);
}

bool AWSAuthV4Signer::SignRequest(Aws::Http::HttpRequest& request, bool signBody) const
{
    AWSCredentials credentials = m_credentialsProvider->GetAWSCredentials();

    //don't sign anonymous requests
    if (credentials.GetAWSAccessKeyId().empty() || credentials.GetAWSSecretKey().empty())
    {
        return true;
    }

    if (!credentials.GetSessionToken().empty())
    {
        request.SetAwsSessionToken(credentials.GetSessionToken());
    }

    Aws::String payloadHash(UNSIGNED_PAYLOAD);
    switch(m_payloadSigningPolicy)
    {
        case PayloadSigningPolicy::Always:
            signBody = true;
            break;
        case PayloadSigningPolicy::Never:
            signBody = false;
            break;
        case PayloadSigningPolicy::RequestDependent:
            // respect the request setting
        default:
            break;
    }

    if(signBody || request.GetUri().GetScheme() != Http::Scheme::HTTPS)
    {
        payloadHash.assign(ComputePayloadHash(request));
        if (payloadHash.empty())
        {
            return false;
        }
    }
    else
    {
        AWS_LOGSTREAM_DEBUG(v4LogTag, "Note: Http payloads are not being signed. signPayloads=" << signBody
                << " http scheme=" << Http::SchemeMapper::ToString(request.GetUri().GetScheme()));
    }

    if(m_includeSha256HashHeader)
    {
        request.SetHeaderValue("x-amz-content-sha256", payloadHash);
    }

    //calculate date header to use in internal signature (this also goes into date header).
    DateTime now = GetSigningTimestamp();
    Aws::String dateHeaderValue = now.ToGmtString(LONG_DATE_FORMAT_STR);
    request.SetHeaderValue(AWS_DATE_HEADER, dateHeaderValue);

    Aws::StringStream headersStream;
    Aws::StringStream signedHeadersStream;

    for (const auto& header : CanonicalizeHeaders(request.GetHeaders()))
    {
        if(ShouldSignHeader(header.first))
        {
            headersStream << header.first.c_str() << ":" << header.second.c_str() << NEWLINE;
            signedHeadersStream << header.first.c_str() << ";";
        }
    }

    Aws::String canonicalHeadersString = headersStream.str();
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Header String: " << canonicalHeadersString);

    //calculate signed headers parameter
    Aws::String signedHeadersValue = signedHeadersStream.str();
    //remove that last semi-colon
    if (!signedHeadersValue.empty())
    {
        signedHeadersValue.pop_back();   
    }

    AWS_LOGSTREAM_DEBUG(v4LogTag, "Signed Headers value:" << signedHeadersValue);

    //generate generalized canonicalized request string.
    Aws::String canonicalRequestString = CanonicalizeRequestSigningString(request, m_urlEscapePath);

    //append v4 stuff to the canonical request string.
    canonicalRequestString.append(canonicalHeadersString);
    canonicalRequestString.append(NEWLINE);
    canonicalRequestString.append(signedHeadersValue);
    canonicalRequestString.append(NEWLINE);
    canonicalRequestString.append(payloadHash);

    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Request String: " << canonicalRequestString);

    //now compute sha256 on that request string
    auto hashResult = m_hash->Calculate(canonicalRequestString);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to hash (sha256) request string");
        AWS_LOGSTREAM_DEBUG(v4LogTag, "The request string is: \"" << canonicalRequestString << "\"");
        return false;
    }

    auto sha256Digest = hashResult.GetResult();
    Aws::String cannonicalRequestHash = HashingUtils::HexEncode(sha256Digest);
    Aws::String simpleDate = now.ToGmtString(SIMPLE_DATE_FORMAT_STR);

    Aws::String stringToSign = GenerateStringToSign(dateHeaderValue, simpleDate, cannonicalRequestHash, m_region,
            m_serviceName);
    auto finalSignature = GenerateSignature(credentials, stringToSign, simpleDate);

    Aws::StringStream ss;
    ss << AWS_HMAC_SHA256 << " " << CREDENTIAL << EQ << credentials.GetAWSAccessKeyId() << "/" << simpleDate
        << "/" << m_region << "/" << m_serviceName << "/" << AWS4_REQUEST << ", " << SIGNED_HEADERS << EQ
        << signedHeadersValue << ", " << SIGNATURE << EQ << finalSignature;

    auto awsAuthString = ss.str();
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Signing request with: " << awsAuthString);
    request.SetAwsAuthorization(awsAuthString);
    request.SetSigningAccessKey(credentials.GetAWSAccessKeyId());
    request.SetSigningRegion(m_region);
    return true;
}

bool AWSAuthV4Signer::PresignRequest(Aws::Http::HttpRequest& request, long long expirationTimeInSeconds) const
{
    return PresignRequest(request, m_region.c_str(), expirationTimeInSeconds);
}

bool AWSAuthV4Signer::PresignRequest(Aws::Http::HttpRequest& request, const char* region, long long expirationInSeconds) const
{
    return PresignRequest(request, region, m_serviceName.c_str(), expirationInSeconds);
}

bool AWSAuthV4Signer::PresignRequest(Aws::Http::HttpRequest& request, const char* region, const char* serviceName, long long expirationTimeInSeconds) const
{
    AWSCredentials credentials = m_credentialsProvider->GetAWSCredentials();

    //don't sign anonymous requests
    if (credentials.GetAWSAccessKeyId().empty() || credentials.GetAWSSecretKey().empty())
    {
        return true;
    }

    Aws::StringStream intConversionStream;
    intConversionStream << expirationTimeInSeconds;
    request.AddQueryStringParameter(Http::X_AMZ_EXPIRES_HEADER, intConversionStream.str());

    if (!credentials.GetSessionToken().empty())
    {
        request.AddQueryStringParameter(Http::AWS_SECURITY_TOKEN, credentials.GetSessionToken());
    }

    //calculate date header to use in internal signature (this also goes into date header).
    DateTime now = GetSigningTimestamp();
    Aws::String dateQueryValue = now.ToGmtString(LONG_DATE_FORMAT_STR);
    request.AddQueryStringParameter(Http::AWS_DATE_HEADER, dateQueryValue);

    Aws::StringStream headersStream;
    Aws::StringStream signedHeadersStream;
    for (const auto& header : CanonicalizeHeaders(request.GetHeaders()))
    {
        if(ShouldSignHeader(header.first))
        {
            headersStream << header.first.c_str() << ":" << header.second.c_str() << NEWLINE;
            signedHeadersStream << header.first.c_str() << ";";
        }
    }

    Aws::String canonicalHeadersString = headersStream.str();
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Header String: " << canonicalHeadersString);

    //calculate signed headers parameter
    Aws::String signedHeadersValue(signedHeadersStream.str());
    //remove that last semi-colon
    if (!signedHeadersValue.empty())
    {
        signedHeadersValue.pop_back();
    }

    request.AddQueryStringParameter(X_AMZ_SIGNED_HEADERS, signedHeadersValue);
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Signed Headers value: " << signedHeadersValue);

    Aws::StringStream ss;
    Aws::String simpleDate = now.ToGmtString(SIMPLE_DATE_FORMAT_STR);
    ss << credentials.GetAWSAccessKeyId() << "/" << simpleDate
        << "/" << region << "/" << serviceName << "/" << AWS4_REQUEST;

    request.AddQueryStringParameter(X_AMZ_ALGORITHM, AWS_HMAC_SHA256);
    request.AddQueryStringParameter(X_AMZ_CREDENTIAL, ss.str());
    ss.str("");

    request.SetSigningAccessKey(credentials.GetAWSAccessKeyId());
    request.SetSigningRegion(region);

    //generate generalized canonicalized request string.
    Aws::String canonicalRequestString = CanonicalizeRequestSigningString(request, m_urlEscapePath);

    //append v4 stuff to the canonical request string.
    canonicalRequestString.append(canonicalHeadersString);
    canonicalRequestString.append(NEWLINE);
    canonicalRequestString.append(signedHeadersValue);
    canonicalRequestString.append(NEWLINE);
    if (ServiceRequireUnsignedPayload(serviceName)) {
        canonicalRequestString.append(UNSIGNED_PAYLOAD);
    } else {
        canonicalRequestString.append(EMPTY_STRING_SHA256);
    }
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Request String: " << canonicalRequestString);

    //now compute sha256 on that request string
    auto hashResult = m_hash->Calculate(canonicalRequestString);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to hash (sha256) request string");
        AWS_LOGSTREAM_DEBUG(v4LogTag, "The request string is: \"" << canonicalRequestString << "\"");
        return false;
    }

    auto sha256Digest = hashResult.GetResult();
    auto cannonicalRequestHash = HashingUtils::HexEncode(sha256Digest);

    auto stringToSign = GenerateStringToSign(dateQueryValue, simpleDate, cannonicalRequestHash, region, serviceName);

    auto finalSigningHash = GenerateSignature(credentials, stringToSign, simpleDate, region, serviceName);
    if (finalSigningHash.empty())
    {
        return false;
    }

    //add that the signature to the query string    
    request.AddQueryStringParameter(X_AMZ_SIGNATURE, finalSigningHash);

    return true;
}

bool AWSAuthV4Signer::ServiceRequireUnsignedPayload(const Aws::String& serviceName) const
{
    // S3 uses a magic string (instead of the empty string) for its body hash for presigned URLs as outlined here:
    // https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-query-string-auth.html
    // this is true for PUT, POST, GET, DELETE and HEAD operations.
    // However, other services (for example RDS) implement the specification as outlined here:
    // https://docs.aws.amazon.com/general/latest/gr/sigv4-create-canonical-request.html
    // which states that body-less requests should use the empty-string SHA256 hash.
    return "s3" == serviceName;
}

Aws::String AWSAuthV4Signer::GenerateSignature(const AWSCredentials& credentials, const Aws::String& stringToSign,
        const Aws::String& simpleDate) const
{
    auto key = ComputeHash(credentials.GetAWSSecretKey(), simpleDate);
    return GenerateSignature(stringToSign, key);
}

Aws::String AWSAuthV4Signer::GenerateSignature(const AWSCredentials& credentials, const Aws::String& stringToSign,
        const Aws::String& simpleDate, const Aws::String& region, const Aws::String& serviceName) const
{
    auto key = ComputeHash(credentials.GetAWSSecretKey(), simpleDate, region, serviceName);
    return GenerateSignature(stringToSign, key);
}

Aws::String AWSAuthV4Signer::GenerateSignature(const Aws::String& stringToSign, const ByteBuffer& key) const
{
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Final String to sign: " << stringToSign);

    Aws::StringStream ss;

    auto hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)stringToSign.c_str(), stringToSign.length()), key);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Unable to hmac (sha256) final string");
        AWS_LOGSTREAM_DEBUG(v4LogTag, "The final string is: \"" << stringToSign << "\"");
        return {};
    }

    //now we finally sign our request string with our hex encoded derived hash.
    auto finalSigningDigest = hashResult.GetResult();

    auto finalSigningHash = HashingUtils::HexEncode(finalSigningDigest);
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Final computed signing hash: " << finalSigningHash);

    return finalSigningHash;
}

Aws::String AWSAuthV4Signer::ComputePayloadHash(Aws::Http::HttpRequest& request) const
{
    if (!request.GetContentBody())
    {
        AWS_LOGSTREAM_DEBUG(v4LogTag, "Using cached empty string sha256 " << EMPTY_STRING_SHA256 << " because payload is empty.");
        return EMPTY_STRING_SHA256;
    }

    //compute hash on payload if it exists.
    auto hashResult =  m_hash->Calculate(*request.GetContentBody());

    if(request.GetContentBody())
    {
        request.GetContentBody()->clear();
        request.GetContentBody()->seekg(0);
    }

    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Unable to hash (sha256) request body");
        return {};
    }

    auto sha256Digest = hashResult.GetResult();

    Aws::String payloadHash(HashingUtils::HexEncode(sha256Digest));
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Calculated sha256 " << payloadHash << " for payload.");
    return payloadHash;
}

Aws::String AWSAuthV4Signer::GenerateStringToSign(const Aws::String& dateValue, const Aws::String& simpleDate,
        const Aws::String& canonicalRequestHash, const Aws::String& region, const Aws::String& serviceName) const
{
    //generate the actual string we will use in signing the final request.
    Aws::StringStream ss;

    ss << AWS_HMAC_SHA256 << NEWLINE << dateValue << NEWLINE << simpleDate << "/" << region << "/"
        << serviceName << "/" << AWS4_REQUEST << NEWLINE << canonicalRequestHash;

    return ss.str();
}

ByteBuffer AWSAuthV4Signer::ComputeHash(const Aws::String& secretKey, const Aws::String& simpleDate) const
{
    Utils::Threading::ReaderLockGuard guard(m_partialSignatureLock);
    if (m_currentDateStr != simpleDate || m_currentSecretKey != secretKey)
    {
        guard.UpgradeToWriterLock();
        // check again to prevent racing writers
        if (m_currentDateStr != simpleDate || m_currentSecretKey != secretKey)
        {
            m_currentSecretKey = secretKey;
            m_currentDateStr = simpleDate;
            m_partialSignature = ComputeHash(m_currentSecretKey, m_currentDateStr, m_region, m_serviceName);
        }
    }
    return m_partialSignature;
}

Aws::Utils::ByteBuffer AWSAuthV4Signer::ComputeHash(const Aws::String& secretKey,
        const Aws::String& simpleDate, const Aws::String& region, const Aws::String& serviceName) const
{
    Aws::String signingKey(SIGNING_KEY);
    signingKey.append(secretKey);
    auto hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)simpleDate.c_str(), simpleDate.length()),
            ByteBuffer((unsigned char*)signingKey.c_str(), signingKey.length()));

    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to HMAC (SHA256) date string \"" << simpleDate << "\"");
        return {};
    }

    auto kDate = hashResult.GetResult();
    hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)region.c_str(), region.length()), kDate);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to HMAC (SHA256) region string \"" << region << "\"");
        return {};
    }

    auto kRegion = hashResult.GetResult();
    hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)serviceName.c_str(), serviceName.length()), kRegion);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to HMAC (SHA256) service string \"" << m_serviceName << "\"");
        return {};
    }

    auto kService = hashResult.GetResult();
    hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)AWS4_REQUEST, strlen(AWS4_REQUEST)), kService);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Unable to HMAC (SHA256) request string");
        AWS_LOGSTREAM_DEBUG(v4LogTag, "The request string is: \"" << AWS4_REQUEST << "\"");
        return {};
    }
    return hashResult.GetResult();
}
