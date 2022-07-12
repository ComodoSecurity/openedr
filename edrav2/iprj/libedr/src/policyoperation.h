// 
// Author: Yury Ermakov (20.08.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy operations declaration
///
/// @addtogroup edr
/// @{
#pragma once
#include "policy.h"

namespace cmd {
namespace edr {
namespace policy {
namespace operation {

/// 
/// Enum class for Variant operations.
///
enum class Operation : std::uint8_t
{
	Not = 0,			///< logical NOT
	And = 1,			///< logical AND
	Or = 2,				///< logical OR
	Equal = 3,			///< values are equal
	NotEqual = 4,		///< values are not equal
	Greater = 5,		///< one integer value is greater than another one
	Less = 6,			///< one integer value is less than another one
	Match = 7,			///< a value matches the regex-pattern
	NotMatch = 8,		///< a value not matches the regex-pattern
	Contain = 9,		///< one value contains another
	NotContain = 10,	///< one value doesn't contain another
	IMatch = 11,		///< a value matches (case-incensitive) the regex-pattern
	NotIMatch = 12,		///< a value not matches (case-incensitive) the regex-pattern
	Add = 13,			///< smart addition of two value
	Extract = 14,		///< extract data from the value by specified path
	Has = 15,			///< variant value has the specified path
	NotHas = 16,		///< variant value has not the specified path
	MakeDescriptor = 17,	///< making of an object descriptor
	Unknown = UINT8_MAX
};

///
/// Get operation name string by given code.
///
/// @param operation - a code of the operation.
/// @return The function returns a string with a name of the operation 
/// corresponding to a given code.
///
const char* getOperationString(Operation operation);

///
/// Get operation code by given name.
///
/// @param sOperation - a name of the operation.
/// @return The function returns a code of the operation corresponding 
/// to a given name.
///
Operation getOperationFromString(std::string_view sOperation);

//
//
//
class IOperation : OBJECT_INTERFACE
{
public:
	virtual Variant compile(Context& ctx) const = 0;
};

using OperationPtr = ObjPtr<IOperation>;

} // namespace operation

using Operation = operation::Operation;
using OperationPtr = operation::OperationPtr;

//
//
//
OperationPtr createOperation(Operation operation, Variant vParams, Context& ctx);

} // namespace policy 
} // namespace edr 
} // namespace cmd 

/// @} 
