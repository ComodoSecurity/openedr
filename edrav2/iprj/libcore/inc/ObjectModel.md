# Object model

## Interface declaration
Interface must be based on the `Object` interface.
The `ObjectInterface` macros should be used for this inheritance.

**Example:**
```cpp
class ExampleClassInterface : ObjectInterface
{
public:
	virtual void exampleMethod() = 0;
};
```

## Class declaration

```cpp
class ExampleClass : public ObjectBase<CLSID_ExampleClassId>, public ExampleClassInterface
{
public:
	ExampleClass(); 
	void finalConstruct(Variant vCfg);
	virtual void exampleMethod() override;
};
```

Class must:

  * inherit from `ObjectBase<CLSID>`. The `CLSID` is unique ClassId (uint32_t). `CLSID` can be skiped if class is not registered.
  * inherit all its interfaces.
  * have default constructor (possible implicit), which does not throw exceptions.
  * define all interfaces methods with `override`. 

Class can:

  * have the `void finalConstruct(Variant vCfg)`. It is called during object creation just after default constructor. The method can throw exceptions.

## Library declaration
The library is static library with objects definition.
The library must have:

  * CLSID derlaration
  * Class registration
  * Macros for call registration (CMD_REGISTER_%LibraryName%. for example: CMD_REGISTER_LIBEXAMPLE)

### Class registration
```cpp
CMD_BEGIN_LIBRARY_DEFINITION(ExampleLibrary)
CMD_DEFINE_LIBRARY_CLASS(ExampleClass)
CMD_END_LIBRARY_DEFINITION(ExampleLibrary)
```

### CLSID derlaration
```cpp
CMD_DECLARE_LIBRARY_CLSID(CLSID_ExampleClassId, 0x12345678);
CMD_DECLARE_LIBRARY_CLSID(CLSID_ExampleClassId2, 0x12345679);
#ifdef CMD_REGISTER_LIBEXAMPLE
CMD_IMPORT_LIBRARY_OBJECTS(ExampleLibrary)
#endif // CMD_REGISTER_LIBEXAMPLE
```

## createObject
The `createObject` macros is used for object creation. `ObjPtr<Object> createObject(CLSID [, vCfg]);`
vCfg is parameter which is passed to `finalConstruct`.
Usage:
```cpp
auto pObj = createObject(CLSID_ExampleClassId);
```


The `createObject` macros also support create object without ClassId and registration:
`ObjPtr<ExampleClass> createObject<ExampleClass>([vCfg]);`.
Usage:
```cpp
auto pObj = createObject<ExampleClassId>();
```

## queryInterface
2 function provided for interface quering:

* `queryInterface<RequestedInterface>(pObject)`. It **throws an exception** if interface is not support.
* `queryInterfaceSafe<RequestedInterface>(pObject)`. It **returns empty pointer** if interface is not support.

Usage:

```cpp
ObjPtr<InterfaceOne> pInterfaceOne;
// Some code sets pInterfaceOne value
// Query interface
ObjPtr<InterfaceTwo> pInterfaceTwo = queryInterface<InterfaceTwo>(pInterfaceOne);
// or shorter
auto pInterfaceTwo_2 = queryInterface<InterfaceTwo>(pInterfaceOne);
```


## ObjPtr
`ObjPtr` is smart pointer to universal object.
`ObjPtr` is `std::shared_ptr` and has the same restrictions.

Main usage rules: 

* Pointer to universal object should always be stored, passed, returned as `ObjPtr`.
* Developer should never create `ObjPtr` from raw pointer.
* Developer can get `ObjPtr` to an object:
  * from `createObject` when he create this object;
  * from `getObjPtrFromThis` inside methods of this object;
  * from other `ObjPtr` to this object.

`ObjPtr` can store pointer to:

* the `Object` interface, 
* any interface which is derived from `Object`,
* any class which is derived from `Object`.

Usage:
```cpp
ObjPtr<Object> pObj;
ObjPtr<ExampleInterface> pExampleInterface;
ObjPtr<ExampleClass> pExampleClass;
```

### getPtrFromThis
The `getPtrFromThis` method ObjPtr to `this`.
It should be used to pass/return pointer to the class from its methods.

`getPtrFromThis` can be called in 2 ways:

* `getPtrFromThis()` returns `ObjPtr<Object>`
* `getPtrFromThis(this)` returns `ObjPtr` with same type as `this`

The `getPtrFromThis` is wrapper of `enable_shared_from_this()`.
`getPtrFromThis` should not be used inside constructor.

Usage:
```cpp
// ExampleClass - is universal object
void ExampleClass::callExampleMethod()
{
    ObjPtr<ExampleClass> pExampleClass = getPtrFromThis(this);
    ObjPtr<Object> pObject = getPtrFromThis();
}
```

## Core object
The `Core` object is basis of object model.
The `Core` object is singleton and not universal object.

The `getCore` function return reference to it.
It provides many features and wrappers for access to these features.
In the most cases developer use wrappers and does not touch the `Core` object directly.

The `Core` object features:

* Object registration (Object factory registration). See point "Library declaration"
* Object creation.. See point "createObject"

