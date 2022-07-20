//
// EDRAv2.libcore project
//
// Variant type implementation
//
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
#pragma once
#include "variant_forward.hpp"

namespace cmd {

//
// overload of queryInterface for Variant
//
template <typename DstInterface, typename T, IsSameType<T, variant::Variant, int> = 0>
ObjPtr<DstInterface> queryInterface(const T& vSrc)
{
	ObjPtr<IObject> pObj = vSrc;
	return queryInterface<DstInterface>(pObj);
}

//
// overload of queryInterfaceSafe for Variant
// It return empty pointer 
// * if Variant is not object
// * if Variant is object but not supports interface
//
template <typename DstInterface, typename T, IsSameType<T, variant::Variant, int> = 0>
ObjPtr<DstInterface> queryInterfaceSafe(const T& vSrc)
{
	auto pObj = variant::detail::convertVariantSafe<ObjPtr<IObject>>(vSrc);
	if (!pObj.has_value())
		return {};
	return queryInterfaceSafe<DstInterface>(*pObj);
}

namespace variant {
namespace detail {

template <typename T>
inline Variant createVariant(const T& Val)
{
	Variant v;
	v.setValue(Val);
	return v;
}

//
// Internal converter to type
// Caller must pass supported types only
//
template <typename T>
inline std::optional<T> convertVariantSafe(const Variant& vThis)
{
	return vThis.getValueSafe<T>();
}

//
//
//
inline RawValueType getRawType(const Variant& vThis)
{
	return vThis.m_Value.getType();
}


//
// If Variant contains conrainer (Dictionary, Sequense)
// the function clones it and reassigns its value.
// Other types aren't changed
//
inline void cloneContainer(Variant& v, bool fResolveProxy)
{
	switch (getRawType(v))
	{
		case RawValueType::Dictionary:
		{
			v = std::move(v.getDict()->clone(fResolveProxy));
			break;
		}
		case RawValueType::Sequence:
		{
			v = std::move(v.getSeq()->clone(fResolveProxy));
			break;
		}
		case RawValueType::Object:
		{
			auto pClonable = queryInterfaceSafe<IClonable>(v);
			if (pClonable)
			{
				v = pClonable->clone();
				if(fResolveProxy)
					resolveDataProxy(v, true);
			}
			break;
		}
		case RawValueType::DataProxy:
		{
			Variant vClone = (**convertVariantSafe<std::shared_ptr<IDataProxy>>(v)).clone();
			if (fResolveProxy)
				resolveDataProxy(vClone, true);
			v = vClone;
			break;
		}
	}
}

//
//
//
inline bool resolveDataProxyInt(Variant& v, bool fRecursive)
{
	bool fNeedReplace = false;

	// Resolve DataProxy
	if (getRawType(v) == RawValueType::DataProxy)
	{
		v.getType();
		fNeedReplace = true;
	}

	if (!fRecursive)
	{
		// Do nothing
	}
	else if (v.isDictionaryLike())
	{
		ConstDictionaryIterator endIter;
		for (auto iter = createDictionaryIterator(v, false); iter != endIter; ++iter)
		{
			Variant vValue = iter->second;
			if (resolveDataProxyInt(vValue, true))
				v.put(iter->first, vValue);
		}
	}
	else if (v.isSequenceLike())
	{
		Size nSize = v.getSize();
		for (Size i = 0; i < nSize; ++i)
		{
			Variant vValue = v.get(i);
			if (resolveDataProxyInt(vValue, true))
				v.put(i, vValue);
		}
	}

	return fNeedReplace;
}


//
//
//
inline void resolveDataProxy(Variant& v, bool fRecursive)
{
	(void)resolveDataProxyInt(v, fRecursive);
}


//
//
//
inline ConstDictionaryIterator createDictionaryIterator(const Variant& vVal, bool fUnsafe)
{
	return vVal.createDictionaryIterator(fUnsafe);
}

} // namespace detail


//
//
//
inline bool Variant::isDictionaryLike() const
{
	switch (getType())
	{
		case ValueType::Dictionary:
			return true;
		case ValueType::Object:
			return (queryInterfaceSafe<IDictionaryProxy>(*this) != nullptr);
		default:
			return false;
	}
}

//
//
//
inline bool Variant::isSequenceLike() const
{
	switch (getType())
	{
		case ValueType::Sequence:
			return true;
		case ValueType::Object:
			return (queryInterfaceSafe<ISequenceProxy>(*this) != nullptr);
		default:
			return false;
	}
}

//
//
//
inline void Variant::resolveDataProxy() const
{
	// recursively resolve DataProxy
	while (true)
	{
		auto ppDataProxy = m_Value.getSafe<DataProxyPtr>();
		if (ppDataProxy == nullptr) 
			break;

		Variant vProxiedData = (*ppDataProxy)->getData();

		*const_cast<Variant*>(this) = std::move(vProxiedData);
	}
}


//
//
//
template<typename VariantPtr, typename Fn>
inline static auto Variant::callDictionaryLike(VariantPtr pThis, Fn fn)
{
	switch (pThis->getType())
	{
		case ValueType::Dictionary:
		{
			auto pDict = pThis->m_Value.get<BasicDictionaryPtr>();
			return fn(std::ref(pDict).get());
		}
		case ValueType::Object:
		{
			auto pDictionaryProxy = queryInterfaceSafe<IDictionaryProxy>(*pThis);
			if (pDictionaryProxy != nullptr)
				return fn(std::ref(pDictionaryProxy).get());
		}
	}
	return fn((IDictionaryProxy*)nullptr);
}

//
//
//
template<typename VariantPtr, typename Fn>
inline static auto Variant::callSequenceLike(VariantPtr pThis, Fn fn)
{
	switch (pThis->getType())
	{
		case ValueType::Sequence:
		{
			auto pDict = pThis->m_Value.get<BasicSequencePtr>();
			return fn(std::ref(pDict).get());
		}
		case ValueType::Object:
		{
			auto pDictionaryProxy = queryInterfaceSafe<ISequenceProxy>(*pThis);
			if (pDictionaryProxy != nullptr)
				return fn(std::ref(pDictionaryProxy).get());
		}
	}
	return fn((ISequenceProxy*)nullptr);
}

//
// Clone Variant (create unique stored data)
//
inline Variant Variant::clone(bool fResolveProxy) const
{
	Variant vNew = *this;
	detail::cloneContainer(vNew, fResolveProxy);
	return vNew;
}

//
//
//
inline Variant& Variant::merge(Variant vSrc, MergeMode eMode)
{
	*this = detail::merge(*this, vSrc, eMode);
	return *this;
}


//
//
//
inline ConstDictionaryIterator Variant::createDictionaryIterator(bool fUnsafe) const
{
	switch (getType())
	{
		case ValueType::Dictionary:
		{
			return ConstDictionaryIterator(getDict()->createIterator(fUnsafe));
		}
		case ValueType::Object:
		{
			auto pDictionaryProxy = queryInterfaceSafe<IDictionaryProxy>(*this);
			if (pDictionaryProxy)
				return ConstDictionaryIterator(pDictionaryProxy->createIterator(fUnsafe));
			break;
		}
	}
	error::TypeError(SL, "Type is not support dictionary iteration").throwException();
}

//
// Create iterator object
//
inline ConstSequenceIterator Variant::createSequenceIterator(bool fUnsafe) const
{
	return ConstSequenceIterator(createIterator(fUnsafe));
}

//
// Create iterator object
//
inline std::unique_ptr<IIterator> Variant::createIterator(bool fUnsafe) const
{
	switch (getType())
	{
		case ValueType::Null:
		{
			// null is empty. return end
			return std::make_unique<detail::NullIterator>();
		}
		case ValueType::Integer:
		case ValueType::Boolean:
		case ValueType::String:
		{
			return std::make_unique<detail::ScalarIterator>(*this);
		}
		case ValueType::Object:
		{
			auto pSequenceProxy = queryInterfaceSafe<ISequenceProxy>(*this);
			if (pSequenceProxy != nullptr)
				return pSequenceProxy->createIterator(fUnsafe);

			auto pDictionaryProxy = queryInterfaceSafe<IDictionaryProxy>(*this);
			if (pDictionaryProxy != nullptr)
				return pDictionaryProxy->createIterator(fUnsafe);

			// if object is not ISequenceProxy and not IDictionaryProxy, it is iterated like scalar
			return std::make_unique<detail::ScalarIterator>(*this);
		}
		case ValueType::Sequence:
		{
			return getSeq()->createIterator(fUnsafe);
		}
		case ValueType::Dictionary:
		{
			return getDict()->createIterator(fUnsafe);
		}
	}
	error::TypeError(SL, "Type doesn't support sequence iteration").throwException();
}

//
//
//
inline ConstSequenceIterator Variant::begin() const
{
	return createSequenceIterator();
}

//
//
//
inline ConstSequenceIterator Variant::end() const
{
	return ConstSequenceIterator();
}

//
//
//
inline ConstSequenceIterator Variant::unsafeBegin() const
{
	return createSequenceIterator(true);
}

//
//
//
inline ConstSequenceIterator Variant::unsafeEnd() const
{
	return ConstSequenceIterator();
}

//
//
//
inline auto Variant::getUnsafeIterable() const
{
	typedef decltype(*this) Container;
	struct
	{
		const Container& m_Contaner;
		auto begin() const { return m_Contaner.unsafeBegin(); }
		auto end() const { return m_Contaner.unsafeEnd(); }
	} adaptor{ *this };
	return adaptor;
}

//
//
//
template<typename T, Variant::IsNullType<T, int> >
inline bool Variant::operator==(const T& /*Value*/) const
{
	auto SelfVal = getValueSafe<nullptr_t>();
	if (!SelfVal.has_value()) return false;
	return true;
}

//
//
//
template<typename T, Variant::IsBoolType<T, int> >
inline bool Variant::operator==(const T& Value) const
{
	auto SelfVal = getValueSafe<bool>();
	if (!SelfVal.has_value()) return false;
	return *SelfVal == Value;
}

//
//
//
template<typename T, Variant::IsIntType<T, int> >
inline bool Variant::operator==(const T& Value) const
{
	auto SelfVal = getValueSafe<IntValue>();
	if (!SelfVal.has_value()) return false;
	return *SelfVal == Value;
}

//
//
//
inline bool Variant::operator==(std::string_view sValue) const
{
	auto SelfVal = getValueSafe<StringValuePtr>();
	if (!SelfVal.has_value()) return false;
	return **SelfVal == sValue;
}

//
//
//
inline bool Variant::operator==(std::wstring_view sValue) const
{
	auto SelfVal = getValueSafe<StringValuePtr>();
	if (!SelfVal.has_value()) return false;
	return **SelfVal == sValue;
}

namespace detail {

//
//
//
inline bool compareDictionaryLike(const Variant& v1, const Variant& v2)
{
	if (v1.getSize() != v2.getSize())
		return false;

	ConstDictionaryIterator endIter;
	for (auto iter = createDictionaryIterator(v1, false); iter != endIter; ++iter)
	{
		auto vVal2 = v2.getSafe(iter->first);
		if (!vVal2.has_value())
			return false;
		if (iter->second != *vVal2)
			return false;
	}

	return true;
}

//
//
//
inline bool compareSequenceLike(const Variant& v1, const Variant& v2)
{
	Size nSize = v1.getSize();
	if (v2.getSize() != nSize)
		return false;

	for (Size i = 0; i < nSize; ++i)
		if (v1.get(i) != v2.get(i))
			return false;
	return true;
}

} // namespace detail

//
//
//
inline bool Variant::operator==(const Variant& other) const
{
	switch (getType())
	{
		case ValueType::Null:
			return other.isNull();
		case ValueType::Boolean:
			if (other.getType() != ValueType::Boolean) return false;
			return ( *m_Value.getSafe<bool>() == *other.m_Value.getSafe<bool>());
		case ValueType::Integer:
			if (other.getType() != ValueType::Integer) return false;
			return (*m_Value.getSafe<IntValue>() == *other.m_Value.getSafe<IntValue>());
		case ValueType::String:
			if (other.getType() != ValueType::String) return false;
			return (**m_Value.getSafe<StringValuePtr>() == **other.m_Value.getSafe<StringValuePtr>());
		case ValueType::Sequence:
			if (!other.isSequenceLike()) return false;
			return detail::compareSequenceLike(*this, other);
		case ValueType::Dictionary:
			if (!other.isDictionaryLike()) return false;
			return detail::compareDictionaryLike(*this, other);
		case ValueType::Object:
			if(isDictionaryLike() && other.isDictionaryLike())
				return detail::compareDictionaryLike(*this, other);
			if (isSequenceLike() && other.isSequenceLike())
				return detail::compareSequenceLike(*this, other);
			
			if (other.getType() != ValueType::Object) return false;
			return ((*m_Value.getSafe<ObjPtr<IObject>>()) == (*other.m_Value.getSafe<ObjPtr<IObject>>()));
	}
	return false;
}

//
//
//
inline Size Variant::getSize() const
{
	switch (getType())
	{
		case ValueType::Dictionary:
			return getDict()->getSize();
		case ValueType::Sequence:
			return getSeq()->getSize();
		case ValueType::Integer:
		case ValueType::Boolean:
		case ValueType::String:
			return 1;
		case ValueType::Object:
		{
			auto pSequenceProxy = queryInterfaceSafe<ISequenceProxy>(*this);
			if (pSequenceProxy != nullptr)
				return pSequenceProxy->getSize();

			auto pDictionaryProxy = queryInterfaceSafe<IDictionaryProxy>(*this);
			if (pDictionaryProxy != nullptr)
				return pDictionaryProxy->getSize();

			return 1;
		}
		case ValueType::Null:
			return 0;
	}
	error::TypeError(SL, FMT("getSize() is not applicable for current type <" <<
		this->getType() << ">")).throwException();
}

//
// Checks if variant is empty
//
inline bool Variant::isEmpty() const
{
	switch (getType())
	{
		case ValueType::Null:
			return true;
		case ValueType::Integer:
		case ValueType::Boolean:
		case ValueType::String:
			return false;
		case ValueType::Object:
		{
			auto pSequenceProxy = queryInterfaceSafe<ISequenceProxy>(*this);
			if (pSequenceProxy != nullptr)
				return pSequenceProxy->isEmpty();

			auto pDictionaryProxy = queryInterfaceSafe<IDictionaryProxy>(*this);
			if (pDictionaryProxy != nullptr)
				return pDictionaryProxy->isEmpty();

			return false;
		}
		case ValueType::Dictionary:
			return getDict()->isEmpty();
		case ValueType::Sequence:
			return getSeq()->isEmpty();
	}
	error::TypeError(SL, FMT("empty() is not applicable for current type <" <<
		this->getType() << ">")).throwException();
}

//
//
//
inline void Variant::clear()
{
	switch (getType())
	{
		case ValueType::Dictionary:
		{
			m_Value.get<BasicDictionaryPtr>()->clear();
			return;
		}
		case ValueType::Sequence:
		{
			m_Value.get<BasicSequencePtr>()->clear();
			return;
		}
		case ValueType::Object:
		{
			auto pSequenceProxy = queryInterfaceSafe<ISequenceProxy>(*this);
			if (pSequenceProxy != nullptr)
				return pSequenceProxy->clear();

			auto pDictionaryProxy = queryInterfaceSafe<IDictionaryProxy>(*this);
			if (pDictionaryProxy != nullptr)
				return pDictionaryProxy->clear();
		}
	}
	error::TypeError(SL, FMT("clear() is not applicable for current type <" <<
		this->getType() << ">")).throwException();
}

//
//
//
inline Size Variant::erase(DictionaryIndex Id)
{
	return callDictionaryLike(this, [&Id, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("erase() is not applicable for current type <" <<
				this->getType() << ">. Key: <" << Id << ">")).throwException();
		return pCont->erase(Id);
	});
}

//
//
//
inline Size Variant::erase(SequenceIndex Id)
{
	return callSequenceLike(this, [&Id, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("erase() is not applicable for current type <" <<
				this->getType() << ">")).throwException();
		return pCont->erase(Id);
	});
}

//
//
//
inline bool Variant::has(DictionaryIndex Id) const
{
	return callDictionaryLike(this, [&Id, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("has() is not applicable for current type <" <<
				this->getType() << ">. Key: <" << Id << ">")).throwException();
		return pCont->has(Id);
	});
}

//
//
//
inline Variant Variant::get(DictionaryIndex Id) const
{
	return callDictionaryLike(this, [&Id, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("String keys are not applicable for type <" <<
				this->getType() << ">. Key: <" << Id << ">")).throwException();
		return pCont->get(Id);
	});
}

//
//
//
inline Variant Variant::get(SequenceIndex Id) const
{
	return callSequenceLike(this, [&Id, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("Integer keys are not applicable for type <" <<
				this->getType() << ">")).throwException();
		return pCont->get(Id);
	});
}

//
//
//
inline Variant Variant::get(SequenceIndex Id, const Variant& vDefaultValue) const
{
	return getSafe(Id).value_or(vDefaultValue);
}

//
//
//
inline Variant Variant::get(DictionaryIndex Id, const Variant& vDefaultValue) const
{
	return getSafe(Id).value_or(vDefaultValue);
}

//
//
//
inline std::optional<Variant> Variant::getSafe(SequenceIndex id) const
{
	return callSequenceLike(this, [&id](auto pCont) -> std::optional<Variant>
	{
		if (pCont == nullptr)
			return std::nullopt;
		return pCont->getSafe(id);
	});
}

//
//
//
inline std::optional<Variant> Variant::getSafe(DictionaryIndex id) const
{
	return callDictionaryLike(this, [&id](auto pCont) -> std::optional<Variant>
	{
		if (pCont == nullptr)
			return std::nullopt;
		return pCont->getSafe(id);
	});
}



//
//
//
inline void Variant::put(DictionaryIndex Id, const Variant& vValue)
{
	return callDictionaryLike(this, [&Id, &vValue, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("put() is not applicable for current type <" <<
				this->getType() << ">. Key: <" << Id << ">")).throwException();
		return pCont->put(Id, vValue);
	});
}

//
//
//
inline void Variant::put(SequenceIndex Id, const Variant& vValue)
{
	return callSequenceLike(this, [&Id, &vValue, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("put() is not applicable for current type <" <<
				this->getType() << ">")).throwException();
		return pCont->put(Id, vValue);
	});
}

//
//
//
inline void Variant::push_back(const Variant& vValue)
{
	return callSequenceLike(this, [&vValue, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("push_back() is not applicable for current type <" <<
				this->getType() << ">")).throwException();
		return pCont->push_back(vValue);
	});
}

//
//
//
inline Variant& Variant::operator+=(const Variant& vValue)
{
	push_back(vValue);
	return *this;
}

//
//
//
inline void Variant::setSize(Size nNewSize)
{
	return callSequenceLike(this, [&nNewSize, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("setSize() is not applicable for current type <" <<
				this->getType() << ">")).throwException();
		return pCont->setSize(nNewSize);
	});
}

//
//
//
inline void Variant::insert(SequenceIndex Id, const Variant& vValue)
{
	return callSequenceLike(this, [&Id, &vValue, this](auto pCont)
	{
		if (pCont == nullptr)
			error::TypeError(SL, FMT("insert() is not applicable for current type <" <<
				this->getType() << ">")).throwException();
		return pCont->insert(Id, vValue);
	});
}



} // namespace variant
} // namespace cmd


//////////////////////////////////////////////////////////////////////////
//
// Catch extension
//

// If catch2 is included
// Avoiding catch2 recursion
#ifdef CATCH_VERSION_MAJOR
namespace Catch {
template<>
struct is_range<cmd::variant::Variant> {
    static const bool value = false;
};
}
#endif // CATCH_VERSION_MAJOR


