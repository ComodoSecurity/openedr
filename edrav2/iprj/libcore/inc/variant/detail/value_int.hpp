//
// EDRAv2. libcore
// Raw integer type for storing Variant internal data 
// 
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
#pragma once

namespace cmd {
namespace variant {
namespace detail {


//
// Integer value class
//
class IntValue
{
private:
	template<typename T>
	using RemoveRefCV = std::remove_cv_t<typename std::remove_reference_t<T> >;

public:

	//
	// SFINAE checker
	// It checks that IntValue supports conversion into/from Type
	//
	// Usage example:
	//   template<typename T, typename std::enable_if_t<IntValue::IsSupportedType<T>::value>* = nullptr>
	template<typename Type, typename TypeWithoutRefCV = typename RemoveRefCV<Type>>
	struct IsSupportedScalarType
	{
		static constexpr bool value = !std::is_same_v<TypeWithoutRefCV, bool> && // not bool
			(std::is_integral_v<TypeWithoutRefCV> || std::is_enum_v<TypeWithoutRefCV>); // integer or enum
	};

	template<typename Type, typename TypeWithoutRefCV = typename RemoveRefCV<Type>>
	struct IsSupportedType
	{
		static constexpr bool value = IsSupportedScalarType<Type>::value ||
			std::is_same_v<TypeWithoutRefCV, IntValue>;
	};

private:
	
	bool m_fIsSigned = false;

	union ValueStorage
	{
		uint64_t nUnsigned;
		int64_t nSigned;
	};

	ValueStorage m_value = { 0 };

	//
	// internal conversions
	//
	template <typename T, typename std::enable_if_t<IsSupportedType<T>::value>* = nullptr>
	void set(T nValue) noexcept
	{
		static_assert(sizeof(T) <= sizeof(m_value.nUnsigned), "Unsupported type: T::getSize() is greater than 64-bit");
		if constexpr (std::is_signed_v<T>)
		{
			m_fIsSigned = true;
			m_value.nSigned = static_cast<int64_t>(nValue);
		}
		else
		{
			m_fIsSigned = false;
			m_value.nUnsigned = static_cast<uint64_t>(nValue);
		}
	}

	//
	//
	//
	template <typename T, typename std::enable_if_t<IsSupportedType<T>::value>* = nullptr>
	T get() const
	{
		try
		{
		
			if constexpr (std::is_signed_v<T>)
			{
				if (m_fIsSigned)
				{
					// optimization if T is int64_t
					if constexpr (std::is_same_v<T, int64_t>)
						return m_value.nSigned;
					else
					// check conversion
						return boost::numeric_cast<T>(m_value.nSigned);
				}
				return boost::numeric_cast<T>(m_value.nUnsigned);
			}
			else
			{
				if (m_fIsSigned)
				{
					// Allow read -1 as unsigned
					// TODO: skip error when negative convert to unsigned
					if (m_value.nSigned == -1)
						return static_cast<T>(-1);
					return boost::numeric_cast<T>(m_value.nSigned);
				}
				// optimization if T is uint64_t
				if constexpr (std::is_same_v<T, uint64_t>)
					return m_value.nUnsigned;
				else
					return boost::numeric_cast<T>(m_value.nUnsigned);
			}
		}
		catch (std::bad_cast& e)
		{
			cmd::error::ArithmeticError(SL, FMT("Can't cast Variant to requested integer type: " << e.what())).throwException();
		}
	}

public:
	// Default constructor. Value = unsigned 0
	IntValue() : m_fIsSigned(false) {}

	// Copy from IntValue 
	IntValue(const IntValue&) = default;

	// Constructor from integer.
	template<typename T, std::enable_if_t<IsSupportedScalarType<T>::value, int> = 0>
	IntValue(T nValue)
	{
		set(nValue);
	}

	// Assignment from IntValue
	IntValue& operator=(const IntValue& other) = default;
	/*{
		m_fIsSigned = other.m_fIsSigned;
		m_value.nUnsigned = other.m_value.nUnsigned;
		return *this;
	}*/
	
	//
	//
	//
	template<typename T, std::enable_if_t <IsSupportedScalarType<T>::value, int> = 0>
	IntValue& operator=(const T& nValue)
	{
		set(nValue);
		return *this;
	}

	//
	//
	//
	template<typename T, std::enable_if_t <IsSupportedScalarType<T>::value, int> = 0>
	operator T() const
	{
		return get<T>();
	}

	//
	//
	//
	bool operator==(const IntValue& other) const noexcept
	{
		// both are sign or both are unsigned
		if (this->m_fIsSigned == other.m_fIsSigned)
			return this->m_value.nUnsigned == other.m_value.nUnsigned;

		// this is negative (other is unsigned)
		if (this->m_fIsSigned && this->m_value.nSigned < 0)
			return false;

		// other is negative (this is unsigned)
		if (other.m_fIsSigned && other.m_value.nSigned < 0)
			return false;
		
		// both are positive
		return this->m_value.nUnsigned == other.m_value.nUnsigned;
	}

	//
	//
	//
	bool operator<(const IntValue& other) const noexcept
	{
		// both are signed
		if (this->m_fIsSigned && other.m_fIsSigned)
			return this->m_value.nSigned < other.m_value.nSigned;

		// both are unsigned
		if (!this->m_fIsSigned && !other.m_fIsSigned)
			return this->m_value.nUnsigned < other.m_value.nUnsigned;

		// this is negative (other is unsigned)
		if (this->m_fIsSigned && this->m_value.nSigned < 0)
			return true;

		// other is negative (this is unsigned)
		if (other.m_fIsSigned && other.m_value.nSigned < 0)
			return false;
		
		// both are positive
		return this->m_value.nUnsigned < other.m_value.nUnsigned;
	}

	//
	//
	//
	bool operator>(const IntValue& other) const noexcept
	{
		// both are signed
		if (this->m_fIsSigned && other.m_fIsSigned)
			return this->m_value.nSigned > other.m_value.nSigned;

		// both are unsigned
		if (!this->m_fIsSigned && !other.m_fIsSigned)
			return this->m_value.nUnsigned > other.m_value.nUnsigned;

		// this is negative (other is unsigned)
		if (this->m_fIsSigned && this->m_value.nSigned < 0)
			return false;

		// other is negative (this is unsigned)
		if (other.m_fIsSigned && other.m_value.nSigned < 0)
			return true;
		
		// both are positive
		return this->m_value.nUnsigned > other.m_value.nUnsigned;
	}

	//
	//
	//
	template<typename T, std::enable_if_t <IsSupportedScalarType<T>::value, int> = 0>
	bool operator==(const T& nValue) const
	{
		return IntValue(nValue) == *this;
	}

	//
	//
	//
	template<typename T, std::enable_if_t <IsSupportedScalarType<T>::value, int> = 0>
	bool operator!=(const T& nValue) const
	{
		return IntValue(nValue) != *this;
	}

	//
	//
	//
	template<typename T, std::enable_if_t <IsSupportedScalarType<T>::value, int> = 0>
	bool operator<(const T& nValue) const
	{
		return *this < IntValue(nValue);
	}

	//
	//
	//
	template<typename T, std::enable_if_t <IsSupportedScalarType<T>::value, int> = 0>
	bool operator>(const T& nValue) const
	{
		return *this > IntValue(nValue);
	}

	//
	// Returns a sign flag
	//
	bool isSigned()
	{
		return m_fIsSigned;
	}

	//
	// returns signed (without checking)
	//
	int64_t asSigned()
	{
		return m_value.nSigned;
	}

	//
	// returns unsigned (without checking)
	//
	uint64_t asUnsigned()
	{
		return m_value.nUnsigned;
	}
};

} // namespace detail
} // namespace variant
} // namespace cmd
