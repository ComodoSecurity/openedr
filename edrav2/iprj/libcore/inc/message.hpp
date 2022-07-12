//
// EDRAv2.libcore project
//
// Author: Denis Bogdanov (17.01.2019)
// Reviewer: TODO:
//
/// @file Declaration of all supported messages
///
/// \addtogroup commonFunctions Common functions
///
/// @{
#pragma once

namespace cmd {

//
// Put the definition of new global message here
//
namespace Message {
	constexpr char AppStarted[] = "AppStarted";
	constexpr char AppFinishing[] = "AppFinishing";
	constexpr char AppFinished[] = "AppFinished";
	constexpr char RequestAppRestart[] = "RequestAppRestart";
	constexpr char RequestAppUninstall[] = "RequestAppUninstall";
	constexpr char RequestAppUpdate[] = "RequestAppUpdate";
	constexpr char RequestPolicyUpdate[] = "RequestPolicyUpdate";
	constexpr char CloudConfigurationIsChanged[] = "CloudConfigurationIsChanged";
	constexpr char PolicyIsUpdated[] = "PolicyIsUpdated";
	
	//const char* CloudIsAvailable = "CloudIsAvailable";
}

//////////////////////////////////////////////////////////////////////////
//
// MessageProcessor
//
//////////////////////////////////////////////////////////////////////////

///
/// MessageProcessor
///
/// MessageProcessor provides message processing: broadcasting and subscribing.
///
/// It is Core object (singleton and created at program startup).
///
/// @sa cmd::sendMessage
/// @sa cmd::subscribeOnMessage
/// @sa cmd::unsubscribeFromMessage
///
class ICommand;

namespace detail {

///
/// MessageProcessor interface
///
class IMessageProcessor
{
protected:
	// Interface can not be destroyed externally
	virtual ~IMessageProcessor() {}

public:

	/// @sa cmd::sendMessage
	virtual void sendMessage(std::string_view sMessageId, Variant vMessageData) = 0;

	/// @sa cmd::subscribeOnMessage
	virtual void subscribe(std::string_view sMessageId, ObjPtr<ICommand> pCallback,
		std::string_view sSubscriptionId) = 0;

	/// @sa cmd::unsubscribeFromMessage
	virtual void unsubscribe(std::string_view sSubscriptionId) = 0;

	/// @sa cmd::unsubscribeAll
	virtual void unsubscribeAll() = 0;

};

///
/// Returns global MessageProcessor
///
extern IMessageProcessor& getMessageProcessor();

} // namespace detail

///
/// Send message to subscribers
///
/// @param sId Message id
/// @param vData additional data
///
inline void sendMessage(std::string_view sId, Variant vData = Variant())
{
	return detail::getMessageProcessor().sendMessage(sId, vData);
}

///
/// Message subscription
///
/// @param sId Message for subscription.
/// @param pCallback Callback
/// @param sSubscriptionId [opt] Identificator for unsubscription
///
/// **pCallback**  
/// When message is sent the `pCallback` is called with Dictionary `params`. 
/// The `params` contains following fields:
///   * "id" - message id. The `sId` parameter of cmd::sendMessage.
///   * "data" [opt] - message data. The `vData` parameter of cmd::sendMessage. It is specified if vData is not null.
/// Exception from Callback is logged and skipped
///
/// **sSubscriptionId**  
/// For unique sSubscriptionId possible a usage of unique object ID of ICommandProcessor object (IObject::getId) 
/// 
inline void subscribeToMessage(std::string_view sId, ObjPtr<ICommand> pCallback, std::string_view sSubscriptionId = "")
{
	return detail::getMessageProcessor().subscribe(sId, pCallback, sSubscriptionId);
}

///
/// Message unsubscription
///
/// @param sSubscriptionId Identificator of subscription
///
inline void unsubscribeFromMessage(std::string_view sSubscriptionId)
{
	return detail::getMessageProcessor().unsubscribe(sSubscriptionId);
}

///
/// Overall message unsubscription
///
inline void unsubscribeAll()
{
	return detail::getMessageProcessor().unsubscribeAll();
}

} // namespace cmd
/// @}
