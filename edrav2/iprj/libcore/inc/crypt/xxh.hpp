//
// edrav2.libcore project
// 
// Author: Denis Kroshin (28.12.2018)
// Reviewer: xxx (29.01.2019)
//

///
/// @file CRC32 implementation
///
#pragma once

namespace cmd {
namespace crypt {
namespace xxh {

///
/// Hash calculator class
/// It is used with crypt::updateHash() and crypt::getHash() 
///
class Hasher
{
private:
	xxh::hash_state64_t m_ctx;
public:
	Hasher() = default;

	typedef uint64_t Hash;
	static constexpr size_t c_nBlockLen = 1;
	static constexpr size_t c_nHashLen = 8;

	void update(const void* pBuffer, size_t nSize)
	{
		if (m_ctx.update(pBuffer, nSize) != xxh::error_code::ok)
			error::ArithmeticError("Can't calculate xxHash").throwException();
	}

	Hash finalize()
	{
		return m_ctx.digest();
	}
};

constexpr size_t c_nHashLen = Hasher::c_nHashLen;
typedef Hasher::Hash Hash;

///
/// Common function for fast hash calculating 
/// Pass all params to crypt::updateHash()
/// Use crypt::updateHash() directly for more complex calculation 
///
template<class... Args>
auto getHash(Args&&... args)
{
	Hasher hasher;
	crypt::updateHash(hasher, std::forward<Args>(args)...);
	return hasher.finalize();
}

} // namespace xxh
} // namespace crypt
} // namespace cmd
