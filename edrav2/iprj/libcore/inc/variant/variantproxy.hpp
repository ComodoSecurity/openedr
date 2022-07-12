//
// EDRAv2.libcore
// 
// Author: Yury Ermakov (08.01.2019)
// Reviewer:
//

///
/// @file DictionaryProxy and SequenceProxy classes declaration
///

///
/// @addtogroup basicDataObjects Basic data objects
/// @{

#pragma once

namespace cmd {
namespace variant {

///
/// Type for proxy creation callback function (both for Dictionary and SequenceProxy)
///
typedef std::function<Variant (Variant)> CreateProxyCallback;

///
/// Type for pre-modify callback function (for DictionaryProxy)
///
/// This function is called before any modification of DictionaryProxy's 
/// internal value.
///
/// @param sKeyName A name of the key to be modified.
/// @param Value Value to be put into a dictionary.
/// @return Value New value. 
/// @throw error::InvalidUsage In case of read-only dictionary.
///
typedef std::function<Variant (const DictionaryKeyRefType&, Variant)> DictionaryPreModifyCallback;

///
/// Type for post-modify callback function (for DictionaryProxy)
///
/// This function is called after any modification of DictionaryProxy's
/// internal value.
///
/// @param sKeyName A name of the key to be modified.
/// @param Value Value to be put into a dictionary.
///
typedef std::function<void (const DictionaryKeyRefType&, Variant)> DictionaryPostModifyCallback;

///
/// Type for pre-clear callback function (for DictionaryProxy)
///
/// This function is called before the call of clear() function of 
/// DictionaryProxy.
///
/// @throw error::InvalidUsage In case of read-only dictionary.
///
typedef std::function<void (void)> DictionaryPreClearCallback;

///
/// Type for post-clear callback function (for DictionaryProxy)
///
/// This function is called after the call of clear() function of 
/// DictionaryProxy.
///
typedef std::function<void (void)> DictionaryPostClearCallback;

///
/// DictionaryProxy object creation
///
/// @param vValue Variant to be wrapped by proxy.
/// @param fnPreModify Pre-modify callback function. Is called before
///   adding or changing of any item inside wrapped Variant (e.g. put(),
///   push_back(), ...). For more details, see DirectoryPreModifyCallback.
/// @param fnPostModify Post-modify callback function. Is called after
///   any operation which modifies a wrapped Variant (e.g. put(), insert(),
///   clear(), ...). For more details, see DictionaryPostModifyCallback.
/// @param fnCreateDictionaryProxy Callback function for DictionaryProxy
///  creation.
/// @param fnCreateSequenceProxy Callback function for SequenceProxy
///  creation.
/// @throw error::TypeError vValue is not a Dictionary or an object which
///  supports IDicionaryProxy interface.
/// @return Variant
///
Variant createDictionaryProxy(Variant vValue,
	DictionaryPreModifyCallback fnPreModify, DictionaryPostModifyCallback fnPostModify,
	DictionaryPreClearCallback fnPreClear, DictionaryPostClearCallback fnPostClear,
	CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProxy);

///
/// Type for pre-modify callback function (for SequenceProxy)
///
/// This function is called before any modification of SequenceProxy's 
/// internal value.
///
/// @param nIndex An index of element to be modified.
/// @param Value Value to be put into a sequence.
/// @return Variant New value. 
/// @throw error::InvalidUsage In case of read-only sequence. 
///
typedef std::function<Variant (Size, Variant)> SequencePreModifyCallback;

///
/// Type for post-modify callback function (for DictionaryProxy)
///
/// This function is called after any modification of DictionaryProxy's
/// internal value.
///
/// @param nIndex An index of element to be modified.
/// @param Value Value to be put into a sequence.
///
typedef std::function<void (Size, Variant)> SequencePostModifyCallback;

///
/// Type for pre-resize callback function (for SequenceProxy)
///
/// This function is called before the call of setSize() or clear()
/// functions of SequenceProxy.
///
/// @throw error::InvalidUsage In case of read-only sequence. 
///
typedef std::function<Size (Size)> SequencePreResizeCallback;

///
/// Type for post-resize callback function (for SequenceProxy)
///
/// This function is called after the call of setSize() or clear()
/// functions of SequenceProxy.
///
typedef std::function<void (Size)> SequencePostResizeCallback;

///
/// SequenceProxy object creation
///
/// @param vValue Variant to be wrapped by proxy.
/// @param fnPreModify Pre-modify callback function.
/// @copydetails SequencePreModifyCallback
/// @param fnPostModify Post-modify callback function.
/// @copydetails SequencePostModifyCallback
/// @param fnPreResize Pre-resize callback function. 
/// @copydetails SequencePreResizeCallback
/// @param fnPostResize Post-resize callback function.
/// @copydetails SequencePostResizeCallback
/// @param fnCreateDictionaryProxy Callback function for DictionaryProxy
/// creation.
/// @param fnCreateSequenceProxy Callback function for SequenceProxy creation.
/// @return Variant
/// @throw error::TypeError vValue is not a Sequence or an object which
/// supports ISequenceProxy interface.
///
Variant createSequenceProxy(Variant vValue,
	SequencePreModifyCallback fnPreModify, SequencePostModifyCallback fnPostModify,
	SequencePreResizeCallback fnPreResize, SequencePostResizeCallback fnPostResize,
	CreateProxyCallback fnCreateDictionaryProxy, CreateProxyCallback fnCreateSequenceProx);

} // namespace variant
} // namespace cmd 

/// @}
