//
//  EDRAv2. Unit test
//    Core object.
//
#include "pch.h"

using namespace openEdr;

//////////////////////////////////////////////////////////////////////////
//
// ObjectManager
//

TEST_CASE("ObjectManager.getObjectManager")
{
	using namespace openEdr;

	auto& ObjectManager1 = getObjectManager();
	auto& ObjectManager2 = getObjectManager();

	REQUIRE(&ObjectManager1 == &ObjectManager2);
}


TEST_CASE("ObjectManager.generateObjectId", "[openEdr::Core]")
{
	using namespace openEdr;

	{
		auto ObjectId1 = getObjectManager().generateObjectId();
		auto ObjectId2 = getObjectManager().generateObjectId();

		REQUIRE(ObjectId1 != ObjectId2);
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Object registration and creation
//


//
// ClassId declaration
//
inline constexpr char CLSID_ClassWithoutFinalConstructStr[] = "0x38656F51";
CMD_DECLARE_LIBRARY_CLSID(CLSID_ClassWithFinalConstruct, 0x38656F50);
CMD_DECLARE_LIBRARY_CLSID(CLSID_ClassWithoutFinalConstruct, 0x38656F51);
CMD_DECLARE_LIBRARY_CLSID(CLSID_ClassWithErrorInConstructor, 0x38656F52);
CMD_DECLARE_LIBRARY_CLSID(CLSID_ClassWithErrorInFinalConstruct, 0x38656F53);
CMD_DECLARE_LIBRARY_CLSID(CLSID_NotRegisteredClass, 0x38656F54);


namespace openEdr {

//
// Test interface 1
//
class ITestInterface : OBJECT_INTERFACE
{
public:
	virtual bool isConstructorCalled() = 0;
	virtual bool isFinalConstructCalled() = 0;
};

class TestInterfaceImpl : public ITestInterface
{
private:
	bool m_fIsConstructorCalled = false;
	bool m_fIsFinalConstructCalled = false;
public:
	void registerConstructorCall() { m_fIsConstructorCalled = true; }
	void registerFinalConstructCall() { m_fIsFinalConstructCalled = true; }

	virtual bool isConstructorCalled() override { return m_fIsConstructorCalled; }
	virtual bool isFinalConstructCalled() override { return m_fIsFinalConstructCalled; }
};

//
//
//
class ClassWithFinalConstruct : public ObjectBase <CLSID_ClassWithFinalConstruct>, public TestInterfaceImpl
{
public:
	ClassWithFinalConstruct()
	{
		registerConstructorCall();
	}
	void finalConstruct(Variant vCfg)
	{
		registerFinalConstructCall();
	}
};

static_assert(openEdr::detail::HasFinalConstruct<ClassWithFinalConstruct>::value, "openEdr::ClassWithFinalConstruct must have finalConstruct");

//
//
//
class ClassWithoutFinalConstruct : public ObjectBase <CLSID_ClassWithoutFinalConstruct>, public TestInterfaceImpl
{
public:
	ClassWithoutFinalConstruct()
	{
		registerConstructorCall();
	}
};

static_assert(!openEdr::detail::HasFinalConstruct<ClassWithoutFinalConstruct>::value, "openEdr::ClassWithFinalConstruct must not have finalConstruct");

//
//
//
class ClassWithoutRegistration : public ObjectBase<>, public TestInterfaceImpl
{
public:
	ClassWithoutRegistration()
	{
		registerConstructorCall();
	}
};

class CreationTestError : public std::exception
{
public:
	CreationTestError(const char* sInfo) : std::exception(sInfo)
	{
	}
};


//
//
//
class ClassWithErrorInConstructor : public ObjectBase<CLSID_ClassWithErrorInConstructor>
{
public:

	ClassWithErrorInConstructor()
	{
		throw CreationTestError("Error in constructor");
	}
};

//
//
//
class ClassWithErrorInFinalConstruct : public ObjectBase<CLSID_ClassWithErrorInFinalConstruct>
{
public:
	void finalConstruct(Variant vCfg)
	{
		throw CreationTestError("Error in finalConstruct");
	}

};

} // namespace openEdr {

//
// Classes registration
//
CMD_BEGIN_LIBRARY_DEFINITION(TestLibrary)
CMD_DEFINE_LIBRARY_CLASS(ClassWithFinalConstruct)
CMD_DEFINE_LIBRARY_CLASS(ClassWithoutFinalConstruct)
CMD_DEFINE_LIBRARY_CLASS(ClassWithErrorInFinalConstruct)
CMD_DEFINE_LIBRARY_CLASS(ClassWithErrorInConstructor)
CMD_END_LIBRARY_DEFINITION(TestLibrary)



TEST_CASE("ObjectManager.IObject_interface", "openEdr::Core")
{
	using namespace openEdr;

	// getClassId
	{
		auto pObj = createObject(CLSID_ClassWithoutFinalConstruct);
		REQUIRE(pObj->getClassId() == CLSID_ClassWithoutFinalConstruct);
	}

	// getId
	{
		auto pObj1 = createObject(CLSID_ClassWithoutFinalConstruct);
		auto pObj2 = createObject(CLSID_ClassWithoutFinalConstruct);
		REQUIRE_FALSE(pObj1->getId() == pObj2->getId());
	}
}
//regCreateObjectAndLibrary
//createObject_and_library_registration
TEST_CASE("ObjectManager.CreateObject_and_Library_registration", "openEdr::Core")
{
	// FIXME: add sections

	// Create via ClassId with finalConstruct
	{
		auto pObj = queryInterface<ITestInterface>(createObject(CLSID_ClassWithFinalConstruct));
		REQUIRE(pObj->getClassId() == CLSID_ClassWithFinalConstruct);
		REQUIRE(pObj->isConstructorCalled());
		REQUIRE(pObj->isFinalConstructCalled());
	}
	 
	// Create via ClassId without finalConstruct
	{
		auto pObj = queryInterface<ITestInterface>(createObject(CLSID_ClassWithoutFinalConstruct));
		REQUIRE(pObj->getClassId() == CLSID_ClassWithoutFinalConstruct);
		REQUIRE(pObj->isConstructorCalled());
		REQUIRE_FALSE(pObj->isFinalConstructCalled());
	}

	// Create via ClassName
	{
		auto pObj = createObject<ClassWithoutRegistration>();
		REQUIRE(pObj->getClassId() == c_nInvalidClassId);
		REQUIRE(pObj->isConstructorCalled());
		REQUIRE_FALSE(pObj->isFinalConstructCalled());
	}

	// Create via ClassName with FinalConstruct
	{
		ObjPtr<ClassWithFinalConstruct> pObj = createObject<ClassWithFinalConstruct>();
		REQUIRE(pObj->getClassId() == CLSID_ClassWithFinalConstruct);
		REQUIRE(pObj->isConstructorCalled());
		REQUIRE(pObj->isFinalConstructCalled());
	}

	// Error in constructor
	{
		REQUIRE_THROWS_AS([]() {
			auto pObj = createObject(CLSID_ClassWithErrorInConstructor);
		}(), CreationTestError);

		REQUIRE_THROWS_AS([]() {
			auto pObj = createObject<ClassWithErrorInConstructor>();
		}(), CreationTestError);
	}

	// Error in finalConstruct
	{
		REQUIRE_THROWS_AS([]() {
			auto pObj = createObject(CLSID_ClassWithErrorInFinalConstruct);
		}(), CreationTestError);

		REQUIRE_THROWS_AS([]() {
			auto pObj = createObject<ClassWithErrorInFinalConstruct>();
		}(), CreationTestError);
	}

	// Error: Class id not found
	{
		REQUIRE_THROWS_AS([]() {
			static constexpr ClassId c_nUnknownClassId = ClassId(0xFFFFFFF0);
			auto pObj = createObject(c_nUnknownClassId);
		}(), error::InvalidArgument);
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Construct and destruct object (finalConstruct and destructor)
//

//
// ClassId declaration
//
CMD_DECLARE_LIBRARY_CLSID(CLSID_ClassForConstructDestructTest, 0x38656DC0);
CMD_DECLARE_LIBRARY_CLSID(CLSID_StatInfoTestObject, 0x35B24C92);

namespace openEdr {

//
//
//
class StatInfoTestObject : public ObjectBase <CLSID_StatInfoTestObject>
{
public:
};


//
//
//
class ClassForConstructDestructTest : public ObjectBase <CLSID_ClassForConstructDestructTest>
{
	Dictionary m_Dict;

public:
	void finalConstruct(Variant vCfg)
	{
		m_Dict = Dictionary(vCfg);
		m_Dict.put("finalConstruct", true);
	}

	~ClassForConstructDestructTest()
	{
		m_Dict.put("destructor", true);
	}
};

} // namespace openEdr {

//
// Classes registration
//
CMD_DEFINE_LIBRARY_CLASS(StatInfoTestObject)
CMD_DEFINE_LIBRARY_CLASS(ClassForConstructDestructTest)


//
//
//
TEST_CASE("ObjectManager.finalConstruct_param_and_destructor")
{
	// Create via ClassId
	{
		Variant dict = Dictionary();
		REQUIRE(dict.getSize() == 0);
		auto pObj = createObject(CLSID_ClassForConstructDestructTest, dict);
		REQUIRE(dict.getSize() == 1);
		REQUIRE(dict["finalConstruct"] == true);
		pObj.reset();
		REQUIRE(dict.getSize() == 2);
		REQUIRE(dict["destructor"] == true);
	}

	// Create via class
	{
		Variant dict = Dictionary();
		// FIXME: IntelliSense error E0350
		REQUIRE(dict.getSize() == 0);
		auto pObj = createObject<ClassForConstructDestructTest>(dict);
		// FIXME: IntelliSense error E0350
		REQUIRE(dict.getSize() == 1);
		// FIXME: IntelliSense error E0350
		REQUIRE(dict["finalConstruct"] == true);
		pObj.reset();
		// FIXME: IntelliSense error E0350
		REQUIRE(dict.getSize() == 2);
		// FIXME: IntelliSense error E0350
		REQUIRE(dict["destructor"] == true);
	}
}

//
//
//
TEST_CASE("ObjectManager.createObject_from_descriptor")
{
	// FIXME: create test of createObject(Variant)
	// * all valid + some invalid
	// Create via ClassId
	{
		REQUIRE_THROWS_AS([]() {
			(void)createObject(Sequence());
		}(), error::InvalidArgument);
	}
}



//////////////////////////////////////////////////////////////////////////
//
// queryInterface and getObjPtrFromThis
//

namespace openEdr {

//
// Base class of several interfaces
//
class ITestInterface_Base : OBJECT_INTERFACE
{
public:
	virtual void method1() = 0;
};

//
//
//
class ITestInterface_Derived1 : public virtual ITestInterface_Base
{
public:
	virtual void method2() = 0;
};

//
//
//
class ITestInterface_Derived2 : public virtual ITestInterface_Base
{
public:
	virtual void method3() = 0;
};

//
//
//
class ITestInterface_NotImplemented : OBJECT_INTERFACE
{
public:
	virtual void method4() = 0;
};

//
//
//
class ClassForQueryInterfaceTest : public ObjectBase <>, public ITestInterface_Derived1, public ITestInterface_Derived2
{
public:

	virtual void method1() override {};
	virtual void method2() override {};
	virtual void method3() override {};

	auto returnPointerToThis();
};

//
//
//
inline auto ClassForQueryInterfaceTest::returnPointerToThis()
{
	return getPtrFromThis(this);
}


} // namespace openEdr {

//
//
//
TEST_CASE("ObjectManager.getObjPtrFromThis")
{
	// getObjPtrFromThis
	{
		ObjPtr<ClassForQueryInterfaceTest> pObj = createObject<ClassForQueryInterfaceTest>();
		REQUIRE(pObj->returnPointerToThis() == pObj);
	}
}

//
//
//
TEST_CASE("ObjectManager.queryInterface")
{
	// Object -> Implemented Interface
	REQUIRE_NOTHROW([]() {
		ObjPtr<IObject> pObj = createObject<ClassForQueryInterfaceTest >();
		ObjPtr<ITestInterface_Derived1> pInterface = queryInterface<ITestInterface_Derived1>(pObj);
		ObjPtr<ITestInterface_Derived1> pInterfaceSafe = queryInterfaceSafe<ITestInterface_Derived1>(pObj);
		REQUIRE(pInterface == pInterfaceSafe);
		REQUIRE(ObjPtr<IObject>(pInterface) == pObj);
	}());

	// Object -> NotImplemented Interface
	{
		ObjPtr<IObject> pObj = createObject<ClassForQueryInterfaceTest>();
		REQUIRE_NOTHROW([&]() {
			ObjPtr<ITestInterface_NotImplemented> pInterfaceSafe = queryInterfaceSafe<ITestInterface_NotImplemented>(pObj);
			REQUIRE(pInterfaceSafe == nullptr);
		}());

		REQUIRE_THROWS_AS([&]() {
			ObjPtr<ITestInterface_NotImplemented> pInterfaceSafe = queryInterface<ITestInterface_NotImplemented>(pObj);
		}(), std::runtime_error);
	}

	// Base Interface -> Derived Interface
	REQUIRE_NOTHROW([]() {
		ObjPtr<IObject> pObj = createObject<ClassForQueryInterfaceTest>();
		ObjPtr<ITestInterface_Base> pBase = queryInterface<ITestInterface_Base>(pObj);
		ObjPtr<ITestInterface_Derived1> pDerived = queryInterface<ITestInterface_Derived1>(pBase);
		ObjPtr<ITestInterface_Derived1> pDerivedSafe = queryInterfaceSafe<ITestInterface_Derived1>(pBase);

		REQUIRE(pDerived == pDerivedSafe);
		REQUIRE(ObjPtr<IObject>(pDerived) == pObj);
	}());

	// Derived Interface -> Base Interface
	REQUIRE_NOTHROW([]() {
		ObjPtr<IObject> pObj = createObject<ClassForQueryInterfaceTest>();
		ObjPtr<ITestInterface_Derived1> pDerived = queryInterface<ITestInterface_Derived1>(pObj);

		ObjPtr<ITestInterface_Base> pBase = queryInterface<ITestInterface_Base>(pDerived);
		ObjPtr<ITestInterface_Base> pBaseSafe = queryInterfaceSafe<ITestInterface_Base>(pDerived);

		REQUIRE(pBase == pBaseSafe);
		REQUIRE(ObjPtr<IObject>(pBase) == pObj);
	}());

	// Derived1 Interface -> Derived2 Interface
	REQUIRE_NOTHROW([]() {
		ObjPtr<IObject> pObj = createObject<ClassForQueryInterfaceTest>();
		ObjPtr<ITestInterface_Derived1> pDerived1 = queryInterface<ITestInterface_Derived1>(pObj);

		ObjPtr<ITestInterface_Derived2> pDerived2 = queryInterface<ITestInterface_Derived2>(pDerived1);
		ObjPtr<ITestInterface_Derived2> pDerived2Safe = queryInterfaceSafe<ITestInterface_Derived2>(pDerived1);

		REQUIRE(pDerived2 == pDerived2Safe);
		REQUIRE(ObjPtr<IObject>(pDerived2) == pObj);
	}());
}


//
//
//
Size getClassCount(ClassId nClassId)
{
	auto statInfos = getObjectManager().getStatInfo();

	for (auto statInfo : statInfos)
		if (statInfo["clsid"] == nClassId)
			return statInfo["count"];

	return c_nMaxSize;
};

//
//
//
TEST_CASE("ObjectManager.getStatInfo")
{
	SECTION("simple")
	{
		REQUIRE(getClassCount(CLSID_StatInfoTestObject) == 0);
		auto pObj = createObject(CLSID_StatInfoTestObject);
		REQUIRE(getClassCount(CLSID_StatInfoTestObject) == 1);
		auto pObj2 = createObject(CLSID_StatInfoTestObject);
		REQUIRE(getClassCount(CLSID_StatInfoTestObject) == 2);
		pObj.reset();
		REQUIRE(getClassCount(CLSID_StatInfoTestObject) == 1);
		pObj2.reset();
		REQUIRE(getClassCount(CLSID_StatInfoTestObject) == 0);
	}

	SECTION("absent")
	{
		REQUIRE(getClassCount(c_nInvalidClassId) == c_nMaxSize);
	}
}


//////////////////////////////////////////////////////////////////////////
//
// MemoryManager 
//

//
//
//
TEST_CASE("MemoryManager.mallocMem")
{
	{
		auto pMem = allocMem(10);
		REQUIRE(pMem.get() != nullptr);
	}
	
	{
		auto pMem = allocMem<uint32_t>(10);
		REQUIRE(pMem.get() != nullptr);
	}

	{
		auto pMem = allocMem(SIZE_MAX/2);
		REQUIRE(pMem.get() == nullptr);
	}
}

//
//
//
TEST_CASE("MemoryManager.CHECK_IN_SOURCE_LOCATION")
{
	CHECK_IN_SOURCE_LOCATION("XXX", 123456);
	{
		auto pMem = allocMem(10);
		SourceLocation pos = getMemSourceLocation(pMem.get());

#ifdef _DEBUG
		REQUIRE(pos.nLine == 123456);
		REQUIRE(pos.sFileName == std::string("XXX"));
#else
		REQUIRE(pos.nLine == 0);
		REQUIRE(pos.sFileName == std::string(""));
#endif
	}
}

//////////////////////////////////////////////////////////////////////////
//
// CatalogManager
//

//
//
//
TEST_CASE("CatalogManager.getCatalogManager")
{
	using namespace openEdr;

	auto& CatalogManager1 = getCatalogManager();
	auto& CatalogManager2 = getCatalogManager();

	REQUIRE(&CatalogManager1 == &CatalogManager2);
}

//
//
//
TEST_CASE("CatalogManager.getCatalogData")
{
	SECTION("Scalar")
	{
		putCatalogData("a", Variant(1));
		REQUIRE(getCatalogData("a") == 1);
		REQUIRE(*getCatalogDataSafe("a") == 1);
	}
	
	SECTION("NestedDictionary")
	{
		putCatalogData("b", Dictionary({ {"a", 100} }));
		REQUIRE(getCatalogData("b.a") == 100);
		REQUIRE(*getCatalogDataSafe("b.a") == 100);
		REQUIRE(getCatalogData("b.a", Variant(200)) == 100);
		REQUIRE(getCatalogData("b.b", Variant(200)) == 200);
		REQUIRE_FALSE(getCatalogDataSafe("b.b").has_value());

		REQUIRE_THROWS_AS([]() {
			(void)getCatalogData("b.b");
		}(), error::OutOfRange);

		REQUIRE_THROWS_AS([]() {
			(void)getCatalogData("Unknown1.Unknown2");
		}(), error::OutOfRange);
		REQUIRE_FALSE(getCatalogDataSafe("Unknown1.Unknown2").has_value());
	}

	SECTION("NestedSequence")
	{
		putCatalogData("c", Sequence({ 0, 1, 2 }));
		REQUIRE(getCatalogData("c[2]") == 2);
		REQUIRE(*getCatalogDataSafe("c[2]") == 2);
		REQUIRE(getCatalogData("c[2]", Variant(200)) == 2);
		REQUIRE(getCatalogData("c[100]", Variant(200)) == 200);
		REQUIRE_FALSE(getCatalogDataSafe("c[100]").has_value());

		REQUIRE_THROWS_AS([]() {
			(void)getCatalogData("c[100]");
		}(), error::OutOfRange);

		REQUIRE_THROWS_AS([]() {
			(void)getCatalogData("[100]");
		}(), error::TypeError);
	}

	SECTION("DeepHierarchy")
	{
		Variant vData = Dictionary({ 
			{"a", Sequence({
				Dictionary({{"a", 1}}) 
			})},
		});
		putCatalogData("d", vData);
		REQUIRE(getCatalogData("d.a[0].a") == 1);
		REQUIRE(*getCatalogDataSafe("d.a[0].a") == 1);
	}

	SECTION("InvalidPath")
	{
		std::string invalidPaths[] = { "[abc]", "."};

		for (auto& sPath : invalidPaths)
		{
			REQUIRE_THROWS_AS([sPath]() {
				(void)getCatalogData(sPath);
			}(), error::InvalidArgument);
			REQUIRE_THROWS_AS([sPath]() {
				(void)getCatalogData(sPath, Variant());
			}(), error::InvalidArgument);
			REQUIRE_THROWS_AS([sPath]() {
				(void)getCatalogDataSafe(sPath);
			}(), error::InvalidArgument);
		}
	}
}

//
//
//
TEST_CASE("CatalogManager.putCatalogData")
{
	SECTION("simple")
	{
		putCatalogData("a", Variant(3));
		REQUIRE(getCatalogData("a") == 3);
	}

	SECTION("replace")
	{
		putCatalogData("b", Variant(3));
		REQUIRE(getCatalogData("b") == 3);
		putCatalogData("b", Variant(5));
		REQUIRE(getCatalogData("b") == 5);
	}

	SECTION("not_cloning")
	{
		Variant vData = Dictionary({ {"a", 1 }, {"b", 2 } });
		putCatalogData("c", vData);
		Variant vData2 = getCatalogData("c");
		REQUIRE(vData2 == Dictionary({ {"a", 1 }, {"b", 2 } }));

		putCatalogData("c.c", Variant(3));
		Variant vData3 = getCatalogData("c");
		REQUIRE(vData2 == Dictionary({ {"a", 1 }, {"b", 2 }, {"c", 3 } }));
		REQUIRE(vData3 == Dictionary({ {"a", 1 }, {"b", 2 }, {"c", 3 } }));
	}

	SECTION("remove_from_dictionary")
	{
		Variant vData = Dictionary({ {"a", 1 }, {"b", 2 } });
		putCatalogData("d", vData);
		REQUIRE(getCatalogData("d") == Dictionary({ {"a", 1 }, {"b", 2 } }));
		putCatalogData("d.a", Variant());
		REQUIRE(getCatalogData("d") == Dictionary({ {"b", 2 } }));
	}

	SECTION("remove_from_sequence")
	{
		Variant vData = Sequence({ 0, 1 ,2 });
		putCatalogData("d", vData);
		REQUIRE(getCatalogData("d") == Sequence({ 0, 1 ,2 }));
		putCatalogData("d[1]", Variant());
		REQUIRE(getCatalogData("d") == Sequence({ 0, 2 }));
	}

	SECTION("not_existed_path")
	{
		putCatalogData("NotExistedPath1.NotExistedPath2.NotExistedPath3", Variant(10));
		REQUIRE(getCatalogData("NotExistedPath1") == 
			Dictionary({ {"NotExistedPath2", Dictionary({ {"NotExistedPath3", 10} })} }));
	}

	SECTION("invalid_path")
	{
		REQUIRE_THROWS_AS([]() {
			putCatalogData(".", Variant(10));
		}(), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("CatalogManager.initCatalogData")
{
	SECTION("simple")
	{
		putCatalogData("a", nullptr);
		REQUIRE(initCatalogData("a", 1) == 1);
		REQUIRE(initCatalogData("a", 5) == 1);
	}

	SECTION("complex_path")
	{
		putCatalogData("a", nullptr);
		REQUIRE(initCatalogData("a.b", 2) == 2);
		REQUIRE(initCatalogData("a.b", 5) == 2);
		REQUIRE(initCatalogData("a", 3) == Dictionary({ {"b", 2} }));
		REQUIRE(initCatalogData("a.a.c", 3) == 3);
		REQUIRE(initCatalogData("a", 3) == Dictionary({ {"b", 2}, {"a", Dictionary({ {"c", 3} })} }));
	}
}

//
//
//
TEST_CASE("CatalogManager.modifyCatalogData")
{
	SECTION("data_is_absent_without_modification")
	{
		putCatalogData("a", nullptr);
		Variant vOldValue;
		(void)modifyCatalogData("a.b", [&vOldValue](Variant vCurValue) { vOldValue = vCurValue; return std::nullopt; });

		REQUIRE(vOldValue.isNull());
		REQUIRE(getCatalogData("a.b", 1) == 1);
	}

	SECTION("data_is_absent_with_modification")
	{
		putCatalogData("a", nullptr);
		Variant vOldValue;
		Variant vNewValue = 2;
		(void)modifyCatalogData("a.b", [&vOldValue, vNewValue](Variant vCurValue) { vOldValue = vCurValue; return vNewValue; });

		REQUIRE(vOldValue.isNull());
		REQUIRE(getCatalogData("a.b") == 2);
	}

	SECTION("data_is_present_without_modification")
	{
		putCatalogData("a", nullptr);
		putCatalogData("a.b", 2);
		Variant vOldValue;
		(void)modifyCatalogData("a.b", [&vOldValue](Variant vCurValue) { vOldValue = vCurValue; return std::nullopt; });

		REQUIRE(vOldValue == 2);
		REQUIRE(getCatalogData("a.b") == 2);
	}

	SECTION("data_is_present_with_modification")
	{
		putCatalogData("a", nullptr);
		putCatalogData("a.b", 2);
		Variant vOldValue;
		Variant vNewValue = 3;
		(void)modifyCatalogData("a.b", [&vOldValue, vNewValue](Variant vCurValue) { vOldValue = vCurValue; return vNewValue; });

		REQUIRE(vOldValue == 2);
		REQUIRE(getCatalogData("a.b") == 3);
	}
}

//
//
//
TEST_CASE("CatalogManager.clearCatalog")
{
	auto fnScenario = [&]()
	{
		clearCatalog();
		REQUIRE(getCatalogData("").getSize() == 0);
		putCatalogData("a", Variant(3));
		REQUIRE(getCatalogData("").getSize() == 1);
		clearCatalog();
		REQUIRE(getCatalogData("").getSize() == 0);
	};

	REQUIRE_NOTHROW(fnScenario());
}

//////////////////////////////////////////////////////////////////////////
//
// MessageProcessor
//
class ExampleMessageSubscriberError : public error::RuntimeError
{
public:
	ExampleMessageSubscriberError(SourceLocation sourceLocation) :
		error::RuntimeError(sourceLocation) {}
	virtual ~ExampleMessageSubscriberError() {}
};


class ExampleMessageSubscriber : public ObjectBase<>, public ICommandProcessor
{
private:
	Variant m_vLastParam;
	Variant m_seqMessageList;
public:

	void finalConstruct(Variant seqMessageList)
	{
		m_seqMessageList = seqMessageList;
	}

	virtual Variant execute(Variant vCommand, Variant vParams) override
	{
		if (vCommand == "storeMessage")
		{
			m_seqMessageList.push_back(vParams["messageId"]);
			if(vParams.has("data"))
				m_seqMessageList.push_back(vParams["data"]);
		}

		if (vCommand == "throwException")
		{
			ExampleMessageSubscriberError(SL).throwException();
		}

		error::InvalidArgument(SL).throwException();
		// Here is an unreachable code to suppress "error C4716:: must return a value"
		return {};
	}
};


//
//
//
TEST_CASE("MessageProcessor.sendMessage")
{
	using namespace openEdr;
	std::string sSubsriptionId = "ExampleMessageSubscriber";

	SECTION("with_parameters")
	{
		Sequence seqMessageList;
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }),
			}));
		unsubscribeFromMessage(sSubsriptionId);
	}

	SECTION("without_parameters")
	{
		Sequence seqMessageList;
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		sendMessage("ExampleMessage1", Variant());
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1"
			}));
		unsubscribeFromMessage(sSubsriptionId);
	}
}


//
//
//
TEST_CASE("MessageProcessor.subscribeOnMessage")
{
	using namespace openEdr;
	std::string sSubsriptionId = "ExampleMessageSubscriber";

	SECTION("one_subscriber")
	{
		Sequence seqMessageList;
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		sendMessage("ExampleMessage2", Dictionary({ {"a", 2} }));
		sendMessage("ExampleMessage1", Dictionary({ {"a", 3} }));
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }), 
			"ExampleMessage1", Dictionary({ {"a", 3} }),
		}));
		unsubscribeFromMessage(sSubsriptionId);
	}

	SECTION("several_subscribers")
	{
		Sequence seqMessageList;
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		sendMessage("ExampleMessage2", Dictionary({ {"a", 2} }));
		sendMessage("ExampleMessage1", Dictionary({ {"a", 3} }));
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }), "ExampleMessage1", Dictionary({ {"a", 1} }),
			"ExampleMessage1", Dictionary({ {"a", 3} }), "ExampleMessage1", Dictionary({ {"a", 3} }),
		}));
		unsubscribeFromMessage(sSubsriptionId);
	}

	SECTION("different_messages")
	{
		Sequence seqMessageList;
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		subscribeToMessage("ExampleMessage2", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		sendMessage("ExampleMessage2", Dictionary({ {"a", 2} }));
		sendMessage("ExampleMessage1", Dictionary({ {"a", 3} }));
		sendMessage("ExampleMessage3", Dictionary({ {"a", 4} }));
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }),
			"ExampleMessage2", Dictionary({ {"a", 2} }),
			"ExampleMessage1", Dictionary({ {"a", 3} }),
			}));
		unsubscribeFromMessage(sSubsriptionId);
	}

	//SECTION("broadcast_subscriber")
	//{
	//	Sequence seqMessageList;
	//	subscribeToMessage("", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
	//	sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
	//	sendMessage("ExampleMessage2", Dictionary({ {"a", 2} }));
	//	sendMessage("ExampleMessage3", Dictionary({ {"a", 3} }));
	//	CHECK(seqMessageList == Sequence({
	//		"ExampleMessage1", Dictionary({ {"a", 1} }),
	//		"ExampleMessage2", Dictionary({ {"a", 2} }),
	//		"ExampleMessage3", Dictionary({ {"a", 3} }),
	//		}));
	//	unsubscribeFromMessage(sSubsriptionId);
	//}

	SECTION("subscriber_returns_error")
	{
		Sequence seqMessageList;
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "throwException"), sSubsriptionId);
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId);
		CHECK_NOTHROW([]() {
			sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		}());

		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }), "ExampleMessage1", Dictionary({ {"a", 1} }),
			}));
		unsubscribeFromMessage(sSubsriptionId);
	}
}

//
//
//
TEST_CASE("MessageProcessor.unsubscribeFromMessage")
{
	using namespace openEdr;

	SECTION("simple_test")
	{ 
		Sequence seqMessageList;
		std::string sSubsriptionId1 = "ExampleMessageSubscriber1";
		auto pObj = createObject<ExampleMessageSubscriber>(seqMessageList);
		subscribeToMessage("ExampleMessage1", createCommand(pObj, "storeMessage"), sSubsriptionId1);
		CHECK(pObj.use_count() != 1);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		unsubscribeFromMessage(sSubsriptionId1);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 2} }));
		CHECK(pObj.use_count() == 1);
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }),
			}));
	}

	SECTION("many_subscribers_with_the_same_id")
	{
		Sequence seqMessageList;
		std::string sSubsriptionId1 = "ExampleMessageSubscriber1";
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId1);
		subscribeToMessage("ExampleMessage2", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId1);
		//subscribeToMessage("", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId1);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		sendMessage("ExampleMessage2", Dictionary({ {"a", 2} }));
		unsubscribeFromMessage(sSubsriptionId1);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 3} }));
		sendMessage("ExampleMessage2", Dictionary({ {"a", 4} }));
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }), // "ExampleMessage1", Dictionary({ {"a", 1} }),
			"ExampleMessage2", Dictionary({ {"a", 2} }), // "ExampleMessage2", Dictionary({ {"a", 2} }),
			}));
	}

	SECTION("many_subscribers_with_different_id")
	{
		Sequence seqMessageList;
		std::string sSubsriptionId1 = "ExampleMessageSubscriber1";
		std::string sSubsriptionId2 = "ExampleMessageSubscriber2";
		std::string sUnknownSubsriptionId = "UnknownExampleMessageSubscriber";
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId1);
		subscribeToMessage("ExampleMessage1", createCommand(createObject<ExampleMessageSubscriber>(seqMessageList), "storeMessage"), sSubsriptionId2);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		unsubscribeFromMessage(sUnknownSubsriptionId);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 2} }));
		unsubscribeFromMessage(sSubsriptionId1);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 3} }));
		unsubscribeFromMessage(sSubsriptionId2);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 4} }));
		CHECK(seqMessageList == Sequence({
			"ExampleMessage1", Dictionary({ {"a", 1} }), "ExampleMessage1", Dictionary({ {"a", 1} }),
			"ExampleMessage1", Dictionary({ {"a", 2} }), "ExampleMessage1", Dictionary({ {"a", 2} }),
			"ExampleMessage1", Dictionary({ {"a", 3} }),
			}));
	}
}

//
//
//
TEST_CASE("MessageProcessor.unsubscribeAll")
{
	using namespace openEdr;

	auto fnScenario = [&]()
	{
		Sequence seqMessageList;
		std::string sSubsriptionId1 = "ExampleMessageSubscriber1";
		auto pObj = createObject<ExampleMessageSubscriber>(seqMessageList);
		subscribeToMessage("ExampleMessage1", createCommand(pObj, "storeMessage"), sSubsriptionId1);
		REQUIRE(pObj.use_count() != 1);
		sendMessage("ExampleMessage1", Dictionary({ {"a", 1} }));
		unsubscribeAll();
		REQUIRE(pObj.use_count() == 1);
	};

	REQUIRE_NOTHROW(fnScenario());
}

//
// @podpruzhnikov FIXME: Fix this test
//
TEST_CASE("MessageProcessor.load_config_data")
{
	using namespace openEdr;

	Sequence seqMessageList;

	Dictionary vConfigSubscribers({
		{"ExampleConfigMessage1", Sequence({
			Dictionary({{"processor", createObject<ExampleMessageSubscriber>(seqMessageList)}, {"command", "storeMessage"}}),
			Dictionary({{"processor", createObject<ExampleMessageSubscriber>(seqMessageList)}, {"command", "storeMessage"}}),
		})},
		{"ExampleConfigMessage2", Sequence({
			Dictionary({{"processor", createObject<ExampleMessageSubscriber>(seqMessageList)}, {"command", "storeMessage"}}),
		})},
	});
	
	putCatalogData("app.config.messageHandlers", vConfigSubscribers);

	sendMessage("ExampleConfigMessage1", Dictionary({ {"a", 1} }));
	sendMessage("ExampleConfigMessage2", Dictionary({ {"a", 2} }));
	sendMessage("ExampleConfigMessage3", Dictionary({ {"a", 3} }));

	CHECK(seqMessageList == Sequence({
		"ExampleConfigMessage1", Dictionary({ {"a", 1} }), "ExampleConfigMessage1", Dictionary({ {"a", 1} }),
		"ExampleConfigMessage2", Dictionary({ {"a", 2} }),
	}));
}
