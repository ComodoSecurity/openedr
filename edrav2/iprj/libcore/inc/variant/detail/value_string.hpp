//
// EDRAv2. libcore
// Raw string type for storing Variant internal data 
// 
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (20.12.2018)
//
#pragma once

namespace openEdr {
namespace variant {
namespace detail {

//
// String variant value
//
class StringValue
{
private:

	mutable std::string m_sUtf8Value;
	mutable std::wstring m_sWCharValue;
	mutable std::mutex m_mutex;

	//
	// Returns UTF8 string.
	// Converts from Wchar if need.
	//
	const std::string& getUtf8() const
	{
		std::scoped_lock _lock(m_mutex);
		// Has value or both empty
		if(!m_sUtf8Value.empty() || m_sWCharValue.empty())
			return m_sUtf8Value;

		// conversion
		m_sUtf8Value = string::convertWCharToUtf8(m_sWCharValue);
		return m_sUtf8Value;
	}

	//
	// Returns wchar string (it is converted from UTF8 if need)
	//
	const std::wstring& getWChar() const
	{
		std::scoped_lock _lock(m_mutex);
		// Has value or both empty
		if (!m_sWCharValue.empty() || m_sUtf8Value.empty())
			return m_sWCharValue;

		// todo conversion
		m_sWCharValue = string::convertUtf8ToWChar(m_sUtf8Value);
		return m_sWCharValue;
	}

	// Don't support copy/move
	StringValue(const StringValue&) = delete;
	StringValue(StringValue&&) = delete;
	StringValue& operator = (const StringValue& other) = delete;
	StringValue& operator = (StringValue&& other) = delete;

	// Private parameter of constructor
	struct PrivateKey{};

public:
	// "private" constructor supporting std::make_shared 
	// Create may use std::make_shared, but constructor can't be called external
	StringValue(const PrivateKey&) {}

	~StringValue() = default;

	//
	// Creation
	//
	static std::shared_ptr<StringValue> create(std::string_view sUtf8)
	{
		auto pThis = std::make_shared<StringValue>(PrivateKey());
		pThis->m_sUtf8Value = sUtf8;
		return pThis;
	}

	//
	// Creation
	//
	static std::shared_ptr<StringValue> create(std::wstring_view sWChar)
	{
		auto pThis = std::make_shared<StringValue>(PrivateKey());
		pThis->m_sWCharValue = sWChar;
		return pThis;
	}

	//
	// method is public for test usage
	// FIXME: Name isn't obvious
	//
	bool hasWChar() const
	{
		std::scoped_lock _lock(m_mutex);
		// Has value or both empty
		return (!m_sWCharValue.empty() || m_sUtf8Value.empty());
	}

	//
	// method is public for test usage
	//
	bool hasUtf8() const
	{
		std::scoped_lock _lock(m_mutex);
		// Has value or both empty
		return (!m_sUtf8Value.empty() || m_sWCharValue.empty());
	}

	//
	//
	//
	operator std::string_view() const
	{
		return getUtf8();
	}

	//
	//
	//
	operator std::wstring_view() const
	{
		return getWChar();
	}

	//
	// Comparison
	//
	bool operator==(std::string_view sStr) const
	{
		return getUtf8() == sStr;
	}

	//
	// Comparison
	//
	bool operator==(std::wstring_view sStr) const
	{
		return getWChar() == sStr;
	}

	//
	// Comparison
	//
	bool operator==(const StringValue& other) const
	{
		if (hasWChar() && other.hasWChar())
			return getWChar() == other.getWChar();
		return getUtf8() == other.getUtf8();
	}

	//
	// Comparison
	//
	template<typename T>
	bool operator!=(const T& other) const
	{
		return !(operator==(other));
	}
};

} // namespace detail
} // namespace variant
} // namespace openEdr
