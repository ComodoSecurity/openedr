//
// edrav2.libcore project
//
// Author: Yury Podpruzhnikov (15.12.2018)
// Reviewer: Denis Bogdanov (15.01.2019) 
//
///
/// @file Core basic objects and interfaces declaration
///
/// @addtogroup basicObjects Basic objects and interfaces
/// Basic objects and interfaces implements a common object model and are
/// used as the basis on creation of all secondary objects and its further
/// inter-communication.
/// @{
#pragma once
#include "object.hpp"
#include "threadpool.hpp"
#include "memory.hpp"

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// ObjectManager
//
//////////////////////////////////////////////////////////////////////////

//
// Class info is provided by object factory
//
struct ObjectClassInfo
{
	bool fIsService = false;
};

//
//
//
class IObjectFactory
{
protected:
	virtual ~IObjectFactory() {}

public:
	//
	// Method of object creation (called by Core)
	//
	virtual	ObjPtr<IObject> createInstance(Variant vConfig, SourceLocation pos) = 0;

	//
	// Get class info
	//
	virtual	ObjectClassInfo getClassInfo() = 0;
};


///
/// Universal objects registration and creation.
///
/// Is is singleton and created at program startup. It is not universal object.
/// The most of ObjectManager features has global function wrapper (e.g. cmd::createObject). 
/// Use them instead raw ObjectManager object access.
/// ObjectManager object pointer should be get with the getObjectManager() function.
///
class IObjectManager
{
protected:
	// Interface can not be destroyed externally
	virtual ~IObjectManager() {}

public:

	//
	// Generate unique ObjectId which is returned by IObject::GetObjectId()
	//
	virtual ObjectId generateObjectId() = 0;

	//
	// Register factory of universal object
	//
	virtual void registerObjectFactory(ClassId nClassId, IObjectFactory* pObjectFactory) = 0;

	//
	// Create registered object by ClassId
	//
	virtual	ObjPtr<IObject> createInstance(ClassId nClassId, Variant vConfig, SourceLocation pos) = 0;

	//
	// Called from constructor & destructor for object counting
	// It is called even if object was created by class (not nClassId)
	//
	virtual void notifyCreateInstance(ClassId nClassId) = 0;
	virtual void notifyDestroyInstance(ClassId nClassId) = 0;

	//
	// Returns stat info about clsid
	// sequence dict with fields:
	// * clsid [int]
	// * count [int]
	//
	virtual Variant getStatInfo() const = 0;

	//
	// Get class properties by ClassId
	//
	virtual	std::optional<ObjectClassInfo> getClassInfo(ClassId nClassId) const = 0;
};

//
// Returns global ObjectManager
//
extern IObjectManager& getObjectManager();


//////////////////////////////////////////////////////////////////////////
//
// Object and library declaration
//
// A library - is an static library with objects definition
//
//////////////////////////////////////////////////////////////////////////

//
// Register objects of library
// 
// It should be used in main header file of library
//
#define CMD_IMPORT_LIBRARY_OBJECTS(LibraryName)											\
namespace cmd { namespace detail {														\
	bool _InitializeLibrary_##LibraryName();											\
	const bool _g_fRegisterLibrary_##LibraryName = _InitializeLibrary_##LibraryName();	\
}} // cmd.detail


//
// Library declaration and object registration
// It should be used in main cpp file of library
//

//
//
//
#define CMD_BEGIN_LIBRARY_DEFINITION(LibraryName)				\
namespace cmd { namespace detail {								\
	bool _InitializeLibrary_##LibraryName() {return true;}		\
}}

//
//
//
#define CMD_END_LIBRARY_DEFINITION(LibraryName)

//
//
//
#define CMD_DEFINE_LIBRARY_CLASS(ClassName)														\
namespace cmd { namespace detail {																\
	static bool CONCAT_ID(_g_fObjectRegistered, __COUNTER__) = cmd::detail::registerObject<ClassName>();	\
}}

//
// CLSID declaration
// 
#define CMD_DECLARE_LIBRARY_CLSID(CLASS_ID_NAME, CLASS_ID)	\
	static constexpr cmd::ClassId CLASS_ID_NAME = cmd::ClassId(CLASS_ID);



//
// Base class for all Object
//
template<ClassId nClassId = c_nInvalidClassId>
class ObjectBase : OBJECT_INTERFACE
{
public:
	public: static constexpr cmd::ClassId c_nClassId = nClassId;
	
	ObjectBase()
	{
		if constexpr (c_nClassId != c_nInvalidClassId)
			getObjectManager().notifyCreateInstance(c_nClassId);
	}
	
	virtual ~ObjectBase()
	{
		if constexpr (c_nClassId != c_nInvalidClassId)
			getObjectManager().notifyDestroyInstance(c_nClassId);
	}

	// Disable copying and moving
	ObjectBase(const ObjectBase<nClassId>&) = delete;
	ObjectBase(ObjectBase<nClassId>&&) = delete;
	ObjectBase<nClassId>& operator=(const ObjectBase<nClassId>&) = delete;
	ObjectBase<nClassId>& operator=(ObjectBase<nClassId>&&) = delete;
};


namespace detail {

//
// IObject interface implementation
// 
template<typename ObjectClass>
class ObjectImpl : public std::enable_shared_from_this<ObjectImpl<ObjectClass> >, public ObjectClass
{
private:
	static_assert(std::is_base_of_v<IObject, ObjectClass>, "ObjectClass must be inherited from `IObject`");
	const ObjectId m_nObjectId = getObjectManager().generateObjectId();

public:

	//
	//
	//
	virtual ClassId getClassId() const
	{
		return ObjectClass::c_nClassId;
	}

	//
	//
	//
	virtual ObjectId getId() const
	{
		return m_nObjectId;
	}

	//
	//
	//
	virtual ObjPtr<IObject> getPtrFromThis() const
	{
		return const_cast<ObjectImpl<ObjectClass>*>(this)->shared_from_this();
	}
};

//
// SFINAE checker
// It check that class has method finalConstruct
//
template <typename T>
class HasFinalConstruct {
private:
	static int detect(...);  // if no finalConstruct
	template<typename U> static decltype(&U::finalConstruct) detect(const U&);

public:
	static constexpr bool value = !std::is_same<int, decltype(detect(std::declval<T>()))>::value;
};

//
// Object initializer if T has finalConstruct
//
template<typename T, std::enable_if_t<HasFinalConstruct<T>::value, int> = 0>
inline void callFinalConstruct(ObjPtr<T> pObj, const Variant& vConfig)
{
	pObj->finalConstruct(vConfig);
}

//
// Object initializer if T does not have finalConstruct
//
template<typename T, std::enable_if_t<!HasFinalConstruct<T>::value, int> = 0>
inline void callFinalConstruct(ObjPtr<T> pObj, const Variant& /*vConfig*/)
{
	return;
}

//
// Object creator
//
template<typename ObjectClass>
inline ObjPtr<ObjectClass> createObjectImpl(Variant vConfig, SourceLocation pos)
{
	CHECK_IN_SOURCE_LOCATION(pos);
	auto pObj = std::make_shared<ObjectImpl<ObjectClass>>();

	// Separate object allocations from data allocated inside object
	// Data has classId instead of line
	cmd::SourceLocation finalConstructSL{ cmd::SourceLocationTag(),
		"finalConstruct", (int)ObjectClass::c_nClassId, pos.sComponent };
	CHECK_IN_SOURCE_LOCATION(finalConstructSL);
	callFinalConstruct(pObj, vConfig);
	return pObj;
}

//////////////////////////////////////////////////////////////////////////
//
// Object factory support
//
//////////////////////////////////////////////////////////////////////////

//
// Object Factory creator and registrator
//
template <typename ObjectClass>
bool registerObject()
{
	static_assert(std::is_base_of_v<cmd::IObject, ObjectClass>, "Registered object must implement `cmd::IObject` interface");
	constexpr ClassId c_nClassId = ObjectClass::c_nClassId;
	static_assert(c_nClassId != c_nInvalidClassId, "Registered object must specify ClassId");

	class ObjectFactoryImpl : public IObjectFactory
	{
	public:
		virtual	ObjPtr<IObject> createInstance(Variant vConfig, SourceLocation pos) override
		{
			return createObjectImpl<ObjectClass>(vConfig, pos);
		}

		virtual	ObjectClassInfo getClassInfo() override
		{
			return ObjectClassInfo{ std::is_base_of_v<cmd::IService, ObjectClass> };
		}
	};

	static ObjectFactoryImpl g_pFactory;

	getObjectManager().registerObjectFactory(c_nClassId, &g_pFactory);

	return true;
}

//
// Object creation helper
//
struct ObjectCreator
{
private:
	SourceLocation m_pos;

	// Process string and integer clsid
	ObjPtr<IObject> createFromVariantClsid(Variant vClassId, Variant vCfg)
	{
		auto eClassIdType = vClassId.getType();
		ClassId nClassId = c_nInvalidClassId;
		if (eClassIdType == Variant::ValueType::String)
			nClassId = ClassId(std::stoul(std::string(vClassId), nullptr, 16));
		else if (eClassIdType == Variant::ValueType::Integer)
			nClassId = vClassId;
		else
			error::InvalidArgument(m_pos, FMT("Invalid object classId type: " << eClassIdType << ", value: " << vClassId)).throwException();

		return callCreateObject(nClassId, vCfg);
	}

public:
	
	// FIXME: Add testing SourceLocation into unit tests
	ObjectCreator(SourceLocation pos) : m_pos(pos)
	{
	}

	// Create class by ClassId
	ObjPtr<IObject> callCreateObject(ClassId nClassId, Variant vCfg=Variant())
	{
		return getObjectManager().createInstance(nClassId, vCfg, m_pos);
	}

	// Create class from descriptor
	ObjPtr<IObject> callCreateObject(Variant vCfg)
	{
		auto eCfgType = vCfg.getType();
		if (eCfgType == Variant::ValueType::Integer || eCfgType == Variant::ValueType::String)
			return createFromVariantClsid(vCfg, Variant());
		else if (vCfg.isDictionaryLike())
			return createFromVariantClsid(vCfg["clsid"], vCfg);
		else
			error::InvalidArgument(m_pos, FMT("Invalid object descriptor type: " << eCfgType << ", value: " << vCfg)).throwException();
	}

	// Create not registered class
	// Usage: callCreateObject<ClassName>()
	template<typename ObjectClass>
	ObjPtr<ObjectClass> callCreateObject(Variant vCfg = Variant())
	{
		return createObjectImpl<ObjectClass>(vCfg, m_pos);
	}
};

} // namespace detail
 
//
// Main macros to create Objects
//
#define createObject cmd::detail::ObjectCreator(SL).callCreateObject

//////////////////////////////////////////////////////////////////////////
//
// CatalogManager
// Global catalog of objects.
// It stores global data such as:
// * Service and named objects
// * Configuration data
// * Application info
//
// Is is singleton and created at program startup
// It is not universal object
//
//////////////////////////////////////////////////////////////////////////



///
/// Functor-modifier of catalog data. It is used in modifyCatalogData.
///
/// @param vOldValue - current value of requested item. 
/// If the item is absent, vOldValue is null.
/// @return The function returns a new value of requested item or 
///		std::nullopt to skip modification.
///
// FIXME: Mabe better to use reference parameter in this case?
typedef std::function<std::optional<Variant>(Variant /*vCurValue*/)> FnCatalogModifier;

//
//
//
class ICatalogManager
{
protected:
	// Interface can not be destroyed externally
	virtual ~ICatalogManager() {}

public:

	/// @copydoc getCatalogData
	virtual Variant getData(std::string_view sPath) = 0;
	/// @copydoc getCatalogData
	virtual std::optional<Variant> getDataSafe(std::string_view sPath) = 0;
	/// @copydoc putCatalogData
	virtual void putData(std::string_view sPath, Variant vData) = 0;
	/// @copydoc initCatalogData
	virtual Variant initData(std::string_view sPath, Variant vDefault) = 0;
	/// @copydoc modifyCatalogData
	virtual Variant modifyData(std::string_view sPath, FnCatalogModifier fnModifier) = 0;
	/// @copydoc clearCatalog
	virtual void clear() = 0;
};

//
// Returns global ICatalogManager
//
extern ICatalogManager& getCatalogManager();

///
/// Returns data from the catalog with the specified path
/// If data not found, throws exception
/// @param sPath - data path
///
inline Variant getCatalogData(std::string_view sPath)
{
	return getCatalogManager().getData(sPath);
}

///
/// Returns data from the catalog with the specified path.
///
/// @param sPath - data path.
/// @param vDefault - default value.
/// @returns The function returns a Variant with requested data. If data
/// is not found, `vDefault` value is returned.
///
inline Variant getCatalogData(std::string_view sPath, Variant vDefault)
{
	return getCatalogManager().getDataSafe(sPath).value_or(vDefault);
}

///
/// Returns data from the catalog with the specified path.
///
/// @param sPath - data path.
/// @returns The function returns a `std::optional` value which contains
/// a Variant with requested data or `std::nullopt` if no data available
/// on the specified path.
///
inline std::optional<Variant> getCatalogDataSafe(std::string_view sPath)
{
	return getCatalogManager().getDataSafe(sPath);
}

///
/// Adds data into the catalog to the specified path.
///
/// If path does not exist, it creates the path following the putByPath() rules.
///
/// @param sPath - data path.
/// @param vData - added data.
///
inline void putCatalogData(std::string_view sPath, Variant vData)
{
	return getCatalogManager().putData(sPath, vData);
}

///
/// Initializes the catalog data with the specified path.
///
/// If data is already exist, the function returns it.
/// If data is not exist, the function does putCatalogData with vDefault and return vDefault value.
///
/// @param sPath - data path.
/// @param vDefault - default (initializing) data.
///
inline Variant initCatalogData(std::string_view sPath, Variant vDefault)
{
	return getCatalogManager().initData(sPath, vDefault);
}



///
/// Synchronized access (modification) to catalog data.
///
/// @param sPath - data path.
/// @param fnModifier - modifier functor. See FnCatalogModifier.
///
template<typename Fn>
Variant modifyCatalogData(std::string_view sPath, Fn&& fnModifier)
{
	return getCatalogManager().modifyData(sPath, fnModifier);
}


///
/// Clears all the catalog data.
///
inline void clearCatalog()
{
	return getCatalogManager().clear();
}


//////////////////////////////////////////////////////////////////////////
//
// ThreadPool
//

//
// Forward definitions for core functions
//
class ThreadPool;
extern ThreadPool& getCoreThreadPool();
extern ThreadPool& getTimerThreadPool();
extern void shutdownGlobalPools();

namespace detail {

	//
	// Helper interface for accessing to the code thread pool
	//
	class IThreadPoolController
	{
	public:
		virtual ThreadPool& getCoreThreadPool() = 0;
		virtual ThreadPool& getTimerThreadPool() = 0;
	};

} // namespace detail



///
/// Runs the specified functor asynchronously using system thread pool.
///
/// @param[in] func - a functor for async call.
/// @param[in] Args - functor's parameters.
///
template<class Functor, class... Args>
inline void run(Functor&& func, Args&&... args)
{
	getCoreThreadPool().run(func, std::forward<Args>(args)...);
}

///
/// Runs the specified functor asynchronously using system timer thread pool after 
/// exceeding of the specified timeout.
///
/// If functor returned true then it will be called again in the specified timeout.
///
/// @param [in] nDuration - timeout for call (in milliseconds).
/// @param [in] func - a functor for async call.
/// @param [in] Args - functor's parameters.
/// @return The function returns a pointer to timer context object.
///
template<class Functor, class... Args>
[[nodiscard]]
inline ThreadPool::TimerContextPtr runWithDelay(Time nTimeout, Functor&& func,
	Args&&... args)
{
	return getCoreThreadPool().runWithDelay(nTimeout, func, std::forward<Args>(args)...);
}

} // namespace cmd
/// @}
