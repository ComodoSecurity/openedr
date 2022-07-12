//
// edrav2.libcore project
//
// Basic conext aware commands
//
// Author: Yury Podpruzhnikov (03.02.2019)
// Reviewer: Denis Bogdanov (14.03.2019)
//
#pragma once
#include <command.hpp>
#include <objects.h>
#include <common.hpp>

namespace cmd {

// Internal namespace to avoid names errors
namespace ctxoper {

/// 
/// Enum class for Variant operations
///
enum class Operation : std::uint8_t
{
	Not = 0,				///< logical NOT
	And = 1,				///< logical AND
	Or = 2,					///< logical OR
	Equal = 3,				///< values are equal
	Greater = 4,			///< one integer value is greater than another one
	Less = 5,				///< one integer value is less than another one
	Match = 6,				///< a value matches the regex-pattern
	Contain = 7,			///< one value contains another
	Add = 8,				///< smart addition of two value
	Extract = 9,			///< extract data from the value by specified path
	Transform = 10,			///< transform the value according to the schema 
	Replace = 11,			///< replace all string occurrences
	EmplaceTransform = 12,	///< transform the value according to the schema without cloning destination
	EmplaceAdd = 13,		///< replace all string occurrences without cloning destination
	Merge = 14,				///< @see Variant::merge()
	EmplaceMerge = 15,		///< @see Variant::merge() without cloning destination
	Has = 16,				///< arg[0] has specified path
	NotHas = 17,				///< arg[0] has specified path
	Filter = 18,			///< filter data accurding to schema (see Variant::transform  with TransformSourceMode::ReplaceSource)
	NotEqual = 19,			///< values are not equal
	IMatch = 20,			///< a value matches (case-incensitive) the regex-pattern
	NotMatch = 21,			///< a value not matches the regex-pattern
	NotIMatch = 22,			///< a value not matches (case-incensitive) the regex-pattern
	Clone = 23,				///< clone variant. Has optional parameter "resolve", default is false
	LogWarning = 24,		///< log some message as warning
	LogInfo = 25,			///< log some message as information
	//Print = 17,				///< print each elements of args

	/// load Json from file which name is specified in "path".
	/// The "default" field contains a value in case absent or invalid file.
	/// If "default" is absent and error occurred, an exception is thrown
	LoadFile = 30,			 
};

//
// Operation interface
//
class IOperation
{
public:
	virtual ~IOperation() {};

	virtual Variant execute(const Variant& vContext) = 0;
};

//
// Operation ptr
//
typedef std::unique_ptr<IOperation> OperationPtr;

//
// Operation factory
//
OperationPtr createOperation(const Variant& vOperation, const Variant& vConfig);

} // namespace ctxoper;

using namespace ctxoper;

//
//
//
class VariantCtxCmd : public ObjectBase<CLSID_VariantCtxCmd>, 
	public IContextCommand
{
private:
	OperationPtr m_pOperation;

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "operation" [str] - operation id
	///   * "args" [seq, opt] - Arguments. Sequence of ContextAwareValue.
	///   * any additional operation specified parameters
	///
	void finalConstruct(Variant vConfig)
	{
		m_pOperation = createOperation(vConfig.get("operation"), vConfig);
	}

	///
	/// @copydoc IContextCommand::execute()
	///
	virtual Variant execute(Variant vContext, Variant vParam) override
	{
		return m_pOperation->execute(vContext);
	}
};


//
//
//
class VariantCmd : public ObjectBase<CLSID_VariantCmd>, 
	public ICommandProcessor
{
public:

	///
	/// @copydoc ICommandProcessor::execute()
	///
	Variant execute(Variant vCommand, Variant vParams) override
	{
		auto pOperation = createOperation(vCommand, vParams);
		return pOperation->execute(nullptr);
	}
};


} // namespace cmd 
