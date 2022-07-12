//
// edrav2.libcore project
//
// Author: Yury Podpruzhnikov (17.12.2018)
// Reviewer: Denis Bogdanov (18.02.2019)
//
///
/// @file Declaration of the IObject interface
///
/// @addtogroup basicObjects Basic objects and interfaces
/// @{
#pragma once
#include "basic.hpp"
#include "string.hpp"

namespace cmd {

///
/// Object CLSID.
///
/// Special type without implicit conversion into integer.
///
enum class ClassId : std::uint32_t {};

static constexpr ClassId c_nInvalidClassId = ClassId(UINT32_MAX);

//
//
//
inline std::ostream& operator<<(std::ostream& os, const ClassId& classId)
{
	return (os << hex(classId));
}


//
// Type which is returned by IObject::GetObjectId()
//
typedef std::uint32_t ObjectId;

class IObject;

//
// Pointer for universal object
//
template<typename T>
using ObjPtr = std::shared_ptr<T>;

///
/// Weak pointer to universal object.
///
template<typename T>
using ObjWeakPtr = std::weak_ptr<T>;


///
/// Base interface for all universal objects.
///
/// Methods are implemented by createObject.
///
class IObject
{
protected:
	///
	/// Creates a smart pointer to `this` inside its method.
	///
	/// @returns Returned type is pointer to `IObject`.
	///
	virtual ObjPtr<IObject> getPtrFromThis() const = 0;

	///
	/// Creates a smart pointer to `this` inside its method.
	///
	/// Usage: auto pThis = getPtrFromThis(this);
	///
	/// @param pThis - pointer to `this`.
	/// @returns Returned type is the same as `this`.
	///
	template<typename T>
	static ObjPtr<T> getPtrFromThis(T* pThis)
	{
		ObjPtr<IObject> pBase = pThis->getPtrFromThis();
		return ObjPtr<T>(pBase, pThis);
	}

	template<typename T>
	static ObjPtr<T> getPtrFromThis(const T* pThis)
	{
		ObjPtr<IObject> pBase = pThis->getPtrFromThis();
		return ObjPtr<T>(pBase, const_cast<T*>(pThis));
	}

public:
	///
	/// Every object must have virtual destructor.
	///
	virtual ~IObject() = 0 {};

	//
	//
	//
	virtual ClassId getClassId() const = 0;

	///
	/// Get unique object identifier.
	///
	/// @returns The function returns the object's identifier.
	///
	virtual ObjectId getId() const = 0;
};

//
// Macros for `IObject` inheritance declaration
//
// FIXME: May we rename it with I prefix?
//
#define OBJECT_INTERFACE virtual public cmd::IObject


//////////////////////////////////////////////////////////////////////////
//
// queryInterface
//

///
/// Safely query an interface from the object.
///
/// @param pSrc - a smart pointer to the source object.
/// @return The function returns a smart pointer to the object's interface or
///		an empty pointer if the interface is not supported.
///
template <typename DstInterface, typename SrcInterface>
ObjPtr<DstInterface> queryInterfaceSafe(const ObjPtr<SrcInterface>& pSrc)
{
	if (!pSrc)
		return nullptr;
	if constexpr (std::is_base_of_v<DstInterface, SrcInterface>)
		return ObjPtr<DstInterface>(pSrc);
	else
		return std::dynamic_pointer_cast<DstInterface>(pSrc);
}

///
/// Query an interface from object.
///
/// If interface is not supported, it throws error.
///
/// @param pSrc - a smart pointer to the source object.
/// @return The function returns a smart pointer to the object's interface.
///
template <typename DstInterface, typename SrcInterface>
ObjPtr<DstInterface> queryInterface(const ObjPtr<SrcInterface>& pSrc)
{
	ObjPtr<DstInterface> pDst = queryInterfaceSafe<DstInterface, SrcInterface>(pSrc);
	// TODO: replace exception and change type
	if (pDst == nullptr)
		throw std::runtime_error("Interface is not implemented");
	return pDst;
}

///
/// Interface for printing a human-readable string representation of the object.
///
class IPrintable : OBJECT_INTERFACE
{
public:

	///
	/// Prints a human-readable string representation (utf8 encoding).
	///
	/// @return The function returns a text string.
	///
	virtual std::string print() const = 0;
};

///
/// Interface of objects supporting serialization.
///
class ISerializable : OBJECT_INTERFACE
{
public:

	///
	/// Creation of serializable representaion of object.
	///
	/// @return The function returns a Variant with serialized data.
	///
	virtual Variant serialize() = 0;
};

///
/// Output of human-readable representation for IObject.
///
template<class T>
inline std::ostream& operator<<(std::ostream& os, ObjPtr<T> pObj)
{
	if (!pObj)
		return (os << "Object <null>");

	auto pPrintable = queryInterfaceSafe<IPrintable>(pObj);
	if (pPrintable)
		return (os << pPrintable->print());

	return (os << "Object<" << pObj->getClassId() << ":" << hex(pObj->getId()) << ">");
}

} // namespace cmd 

/// @}