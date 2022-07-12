//
// edrav2.libcore project
//
// Common interfaces and declarations
//
// Autor: Yury Podpruzhnikov (17.12.2018)
// Reviewer: Denis Bogdanov (18.01.2019)
//
#pragma once
#include "object.hpp"
#include "variant.hpp"

namespace cmd {

///
/// Interface provides abstract function for loading Variant variable into object.
///
class IDataReceiver : OBJECT_INTERFACE
{
public:

	///
	/// Puts the data to the object.
	///
	/// @param vData - data for adding.
	/// @throw OperationDeclined exeption is thrown then the object can't
	///        process data because of current working mode.
	///
	virtual void put(const Variant& vData) = 0;
};

///
/// Interface provides abstract function for getting variant from object.
///
class IDataProvider : OBJECT_INTERFACE
{
public:

	///
	/// Gets the data from the object and returns it.
	///
	/// @return std::optional object which is valid then the object 
	///         returns valid data.
	///
	virtual std::optional<Variant> get() = 0;
};

///
/// Interface provides abstract functions for matching and replacing strings.
///
class IStringMatcher : OBJECT_INTERFACE
{
public:

	///
	/// Checks if the string value matches in the object's search space.
	///
	/// @param sValue - a string value for matching.
	/// @param fCaseInsensitive - a flag for case-insensitive matching.
	/// @return Returns true if the string matches, false overwise. 
	///
	virtual bool match(std::string_view sValue, bool fCaseInsensitive) const = 0;

	///
	/// Checks if the string value matches in the object's search space.
	///
	/// @param sValue - a string value for matching.
	/// @param fCaseInsensitive - a flag for case-insensitive matching.
	/// @return Returns true if the string matches, false overwise.
	///
	virtual bool match(std::wstring_view sValue, bool fCaseInsensitive) const = 0;

	///
	/// Makes replacements in the string according to the object's rules.
	///
	/// @param sValue - the source string value.
	/// @return Returns the result string.
	///
	virtual std::string replace(std::string_view sValue) const = 0;

	///
	/// Makes replacements in the string according to the object's rules.
	///
	/// @param sValue - the source string value.
	/// @return Returns the result string.
	///
	virtual std::wstring replace(std::wstring_view sValue) const = 0;
};

///
/// Interface provides control on data generatior objects.
///
class IDataGenerator : OBJECT_INTERFACE
{
public:

	///
	/// Starts providing the specified samples set.
	///
	/// @param sName - the name of data set.
	/// @param nCount - the number of events.
	///
	virtual void start(std::string_view sName, Size nCount = c_nMaxSize) = 0;

	///
	/// Stops providing the current data set.
	///
	virtual void stop() = 0;

	///
	/// Stops providing the current data set and free all data.
	///
	virtual void terminate() = 0;
};

///
/// Interface for providing generic object information.
///
class IInformationProvider : OBJECT_INTERFACE
{
public:

	///
	/// Gets a generic object information.
	///
	/// @return Data packet with information.
	///
	virtual Variant getInfo() = 0;
};

///
/// Interface for providing generic object information.
///
class IProgressReporter : OBJECT_INTERFACE
{
public:

	///
	/// Gets progress information.
	///
	/// @return Data packet with information.
	///
	virtual Variant getProgressInfo() = 0;
};

} // namespace cmd 

