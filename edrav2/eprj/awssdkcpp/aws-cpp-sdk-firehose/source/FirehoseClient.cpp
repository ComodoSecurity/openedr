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

#include <aws/core/utils/Outcome.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/client/RetryStrategy.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/core/utils/DNS.h>
#include <aws/core/utils/logging/LogMacros.h>

#include <aws/firehose/FirehoseClient.h>
#include <aws/firehose/FirehoseEndpoint.h>
#include <aws/firehose/FirehoseErrorMarshaller.h>
#include <aws/firehose/model/CreateDeliveryStreamRequest.h>
#include <aws/firehose/model/DeleteDeliveryStreamRequest.h>
#include <aws/firehose/model/DescribeDeliveryStreamRequest.h>
#include <aws/firehose/model/ListDeliveryStreamsRequest.h>
#include <aws/firehose/model/ListTagsForDeliveryStreamRequest.h>
#include <aws/firehose/model/PutRecordRequest.h>
#include <aws/firehose/model/PutRecordBatchRequest.h>
#include <aws/firehose/model/StartDeliveryStreamEncryptionRequest.h>
#include <aws/firehose/model/StopDeliveryStreamEncryptionRequest.h>
#include <aws/firehose/model/TagDeliveryStreamRequest.h>
#include <aws/firehose/model/UntagDeliveryStreamRequest.h>
#include <aws/firehose/model/UpdateDestinationRequest.h>

using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::Firehose;
using namespace Aws::Firehose::Model;
using namespace Aws::Http;
using namespace Aws::Utils::Json;

static const char* SERVICE_NAME = "firehose";
static const char* ALLOCATION_TAG = "FirehoseClient";


FirehoseClient::FirehoseClient(const Client::ClientConfiguration& clientConfiguration) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, Aws::MakeShared<DefaultAWSCredentialsProviderChain>(ALLOCATION_TAG),
        SERVICE_NAME, clientConfiguration.region),
    Aws::MakeShared<FirehoseErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor)
{
  init(clientConfiguration);
}

FirehoseClient::FirehoseClient(const AWSCredentials& credentials, const Client::ClientConfiguration& clientConfiguration) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, Aws::MakeShared<SimpleAWSCredentialsProvider>(ALLOCATION_TAG, credentials),
         SERVICE_NAME, clientConfiguration.region),
    Aws::MakeShared<FirehoseErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor)
{
  init(clientConfiguration);
}

FirehoseClient::FirehoseClient(const std::shared_ptr<AWSCredentialsProvider>& credentialsProvider,
  const Client::ClientConfiguration& clientConfiguration) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, credentialsProvider,
         SERVICE_NAME, clientConfiguration.region),
    Aws::MakeShared<FirehoseErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor)
{
  init(clientConfiguration);
}

FirehoseClient::~FirehoseClient()
{
}

void FirehoseClient::init(const ClientConfiguration& config)
{
  m_configScheme = SchemeMapper::ToString(config.scheme);
  if (config.endpointOverride.empty())
  {
      m_uri = m_configScheme + "://" + FirehoseEndpoint::ForRegion(config.region, config.useDualStack);
  }
  else
  {
      OverrideEndpoint(config.endpointOverride);
  }
}

void FirehoseClient::OverrideEndpoint(const Aws::String& endpoint)
{
  if (endpoint.compare(0, 7, "http://") == 0 || endpoint.compare(0, 8, "https://") == 0)
  {
      m_uri = endpoint;
  }
  else
  {
      m_uri = m_configScheme + "://" + endpoint;
  }
}
CreateDeliveryStreamOutcome FirehoseClient::CreateDeliveryStream(const CreateDeliveryStreamRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return CreateDeliveryStreamOutcome(CreateDeliveryStreamResult(outcome.GetResult()));
  }
  else
  {
    return CreateDeliveryStreamOutcome(outcome.GetError());
  }
}

CreateDeliveryStreamOutcomeCallable FirehoseClient::CreateDeliveryStreamCallable(const CreateDeliveryStreamRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< CreateDeliveryStreamOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->CreateDeliveryStream(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::CreateDeliveryStreamAsync(const CreateDeliveryStreamRequest& request, const CreateDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->CreateDeliveryStreamAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::CreateDeliveryStreamAsyncHelper(const CreateDeliveryStreamRequest& request, const CreateDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, CreateDeliveryStream(request), context);
}

DeleteDeliveryStreamOutcome FirehoseClient::DeleteDeliveryStream(const DeleteDeliveryStreamRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return DeleteDeliveryStreamOutcome(DeleteDeliveryStreamResult(outcome.GetResult()));
  }
  else
  {
    return DeleteDeliveryStreamOutcome(outcome.GetError());
  }
}

DeleteDeliveryStreamOutcomeCallable FirehoseClient::DeleteDeliveryStreamCallable(const DeleteDeliveryStreamRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteDeliveryStreamOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteDeliveryStream(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::DeleteDeliveryStreamAsync(const DeleteDeliveryStreamRequest& request, const DeleteDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteDeliveryStreamAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::DeleteDeliveryStreamAsyncHelper(const DeleteDeliveryStreamRequest& request, const DeleteDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteDeliveryStream(request), context);
}

DescribeDeliveryStreamOutcome FirehoseClient::DescribeDeliveryStream(const DescribeDeliveryStreamRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return DescribeDeliveryStreamOutcome(DescribeDeliveryStreamResult(outcome.GetResult()));
  }
  else
  {
    return DescribeDeliveryStreamOutcome(outcome.GetError());
  }
}

DescribeDeliveryStreamOutcomeCallable FirehoseClient::DescribeDeliveryStreamCallable(const DescribeDeliveryStreamRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DescribeDeliveryStreamOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DescribeDeliveryStream(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::DescribeDeliveryStreamAsync(const DescribeDeliveryStreamRequest& request, const DescribeDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DescribeDeliveryStreamAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::DescribeDeliveryStreamAsyncHelper(const DescribeDeliveryStreamRequest& request, const DescribeDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DescribeDeliveryStream(request), context);
}

ListDeliveryStreamsOutcome FirehoseClient::ListDeliveryStreams(const ListDeliveryStreamsRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return ListDeliveryStreamsOutcome(ListDeliveryStreamsResult(outcome.GetResult()));
  }
  else
  {
    return ListDeliveryStreamsOutcome(outcome.GetError());
  }
}

ListDeliveryStreamsOutcomeCallable FirehoseClient::ListDeliveryStreamsCallable(const ListDeliveryStreamsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListDeliveryStreamsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListDeliveryStreams(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::ListDeliveryStreamsAsync(const ListDeliveryStreamsRequest& request, const ListDeliveryStreamsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListDeliveryStreamsAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::ListDeliveryStreamsAsyncHelper(const ListDeliveryStreamsRequest& request, const ListDeliveryStreamsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListDeliveryStreams(request), context);
}

ListTagsForDeliveryStreamOutcome FirehoseClient::ListTagsForDeliveryStream(const ListTagsForDeliveryStreamRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return ListTagsForDeliveryStreamOutcome(ListTagsForDeliveryStreamResult(outcome.GetResult()));
  }
  else
  {
    return ListTagsForDeliveryStreamOutcome(outcome.GetError());
  }
}

ListTagsForDeliveryStreamOutcomeCallable FirehoseClient::ListTagsForDeliveryStreamCallable(const ListTagsForDeliveryStreamRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListTagsForDeliveryStreamOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListTagsForDeliveryStream(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::ListTagsForDeliveryStreamAsync(const ListTagsForDeliveryStreamRequest& request, const ListTagsForDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListTagsForDeliveryStreamAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::ListTagsForDeliveryStreamAsyncHelper(const ListTagsForDeliveryStreamRequest& request, const ListTagsForDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListTagsForDeliveryStream(request), context);
}

PutRecordOutcome FirehoseClient::PutRecord(const PutRecordRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return PutRecordOutcome(PutRecordResult(outcome.GetResult()));
  }
  else
  {
    return PutRecordOutcome(outcome.GetError());
  }
}

PutRecordOutcomeCallable FirehoseClient::PutRecordCallable(const PutRecordRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutRecordOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutRecord(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::PutRecordAsync(const PutRecordRequest& request, const PutRecordResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutRecordAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::PutRecordAsyncHelper(const PutRecordRequest& request, const PutRecordResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutRecord(request), context);
}

PutRecordBatchOutcome FirehoseClient::PutRecordBatch(const PutRecordBatchRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return PutRecordBatchOutcome(PutRecordBatchResult(outcome.GetResult()));
  }
  else
  {
    return PutRecordBatchOutcome(outcome.GetError());
  }
}

PutRecordBatchOutcomeCallable FirehoseClient::PutRecordBatchCallable(const PutRecordBatchRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutRecordBatchOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutRecordBatch(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::PutRecordBatchAsync(const PutRecordBatchRequest& request, const PutRecordBatchResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutRecordBatchAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::PutRecordBatchAsyncHelper(const PutRecordBatchRequest& request, const PutRecordBatchResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutRecordBatch(request), context);
}

StartDeliveryStreamEncryptionOutcome FirehoseClient::StartDeliveryStreamEncryption(const StartDeliveryStreamEncryptionRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return StartDeliveryStreamEncryptionOutcome(StartDeliveryStreamEncryptionResult(outcome.GetResult()));
  }
  else
  {
    return StartDeliveryStreamEncryptionOutcome(outcome.GetError());
  }
}

StartDeliveryStreamEncryptionOutcomeCallable FirehoseClient::StartDeliveryStreamEncryptionCallable(const StartDeliveryStreamEncryptionRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< StartDeliveryStreamEncryptionOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->StartDeliveryStreamEncryption(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::StartDeliveryStreamEncryptionAsync(const StartDeliveryStreamEncryptionRequest& request, const StartDeliveryStreamEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->StartDeliveryStreamEncryptionAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::StartDeliveryStreamEncryptionAsyncHelper(const StartDeliveryStreamEncryptionRequest& request, const StartDeliveryStreamEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, StartDeliveryStreamEncryption(request), context);
}

StopDeliveryStreamEncryptionOutcome FirehoseClient::StopDeliveryStreamEncryption(const StopDeliveryStreamEncryptionRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return StopDeliveryStreamEncryptionOutcome(StopDeliveryStreamEncryptionResult(outcome.GetResult()));
  }
  else
  {
    return StopDeliveryStreamEncryptionOutcome(outcome.GetError());
  }
}

StopDeliveryStreamEncryptionOutcomeCallable FirehoseClient::StopDeliveryStreamEncryptionCallable(const StopDeliveryStreamEncryptionRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< StopDeliveryStreamEncryptionOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->StopDeliveryStreamEncryption(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::StopDeliveryStreamEncryptionAsync(const StopDeliveryStreamEncryptionRequest& request, const StopDeliveryStreamEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->StopDeliveryStreamEncryptionAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::StopDeliveryStreamEncryptionAsyncHelper(const StopDeliveryStreamEncryptionRequest& request, const StopDeliveryStreamEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, StopDeliveryStreamEncryption(request), context);
}

TagDeliveryStreamOutcome FirehoseClient::TagDeliveryStream(const TagDeliveryStreamRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return TagDeliveryStreamOutcome(TagDeliveryStreamResult(outcome.GetResult()));
  }
  else
  {
    return TagDeliveryStreamOutcome(outcome.GetError());
  }
}

TagDeliveryStreamOutcomeCallable FirehoseClient::TagDeliveryStreamCallable(const TagDeliveryStreamRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< TagDeliveryStreamOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->TagDeliveryStream(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::TagDeliveryStreamAsync(const TagDeliveryStreamRequest& request, const TagDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->TagDeliveryStreamAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::TagDeliveryStreamAsyncHelper(const TagDeliveryStreamRequest& request, const TagDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, TagDeliveryStream(request), context);
}

UntagDeliveryStreamOutcome FirehoseClient::UntagDeliveryStream(const UntagDeliveryStreamRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return UntagDeliveryStreamOutcome(UntagDeliveryStreamResult(outcome.GetResult()));
  }
  else
  {
    return UntagDeliveryStreamOutcome(outcome.GetError());
  }
}

UntagDeliveryStreamOutcomeCallable FirehoseClient::UntagDeliveryStreamCallable(const UntagDeliveryStreamRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< UntagDeliveryStreamOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->UntagDeliveryStream(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::UntagDeliveryStreamAsync(const UntagDeliveryStreamRequest& request, const UntagDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->UntagDeliveryStreamAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::UntagDeliveryStreamAsyncHelper(const UntagDeliveryStreamRequest& request, const UntagDeliveryStreamResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, UntagDeliveryStream(request), context);
}

UpdateDestinationOutcome FirehoseClient::UpdateDestination(const UpdateDestinationRequest& request) const
{
  Aws::Http::URI uri = m_uri;
  Aws::StringStream ss;
  ss << "/";
  uri.SetPath(uri.GetPath() + ss.str());
  JsonOutcome outcome = MakeRequest(uri, request, HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER);
  if(outcome.IsSuccess())
  {
    return UpdateDestinationOutcome(UpdateDestinationResult(outcome.GetResult()));
  }
  else
  {
    return UpdateDestinationOutcome(outcome.GetError());
  }
}

UpdateDestinationOutcomeCallable FirehoseClient::UpdateDestinationCallable(const UpdateDestinationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< UpdateDestinationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->UpdateDestination(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void FirehoseClient::UpdateDestinationAsync(const UpdateDestinationRequest& request, const UpdateDestinationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->UpdateDestinationAsyncHelper( request, handler, context ); } );
}

void FirehoseClient::UpdateDestinationAsyncHelper(const UpdateDestinationRequest& request, const UpdateDestinationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, UpdateDestination(request), context);
}

