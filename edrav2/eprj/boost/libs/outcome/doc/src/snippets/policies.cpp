#include "../../../include/boost/outcome.hpp"

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

//! [abort_policy]
struct abort_policy : outcome::policy::base
{
  template <class Impl> static constexpr void wide_value_check(Impl &&self)
  {
    if(!base::_has_value(std::forward<Impl>(self)))
      std::abort();
  }

  template <class Impl> static constexpr void wide_error_check(Impl &&self)
  {
    if(!base::_has_error(std::forward<Impl>(self)))
      std::abort();
  }

  template <class Impl> static constexpr void wide_exception_check(Impl &&self)
  {
    if(!base::_has_exception(std::forward<Impl>(self)))
      std::abort();
  }
};
//! [abort_policy]

//! [throwing_policy]
template <typename T, typename EC, typename EP> struct throwing_policy : outcome::policy::base
{
  static_assert(std::is_convertible<EC, std::error_code>::value, "only EC = error_code");

  template <class Impl> static constexpr void wide_value_check(Impl &&self)
  {
    if(!base::_has_value(std::forward<Impl>(self)))
    {
      if(base::_has_error(std::forward<Impl>(self)))
        throw std::system_error(base::_error(std::forward<Impl>(self)));
      else
        std::rethrow_exception(base::_exception<T, EC, EP, throwing_policy>(std::forward<Impl>(self)));
    }
  }

  template <class Impl> static constexpr void wide_error_check(Impl &&self)
  {
    if(!base::_has_error(std::forward<Impl>(self)))
    {
      if(base::_has_exception(std::forward<Impl>(self)))
        std::rethrow_exception(base::_exception<T, EC, EP, throwing_policy>(std::forward<Impl>(self)));
      else
        base::_ub(std::forward<Impl>(self));
    }
  }

  template <class Impl> static constexpr void wide_exception_check(Impl &&self)
  {
    if(!base::_has_exception(std::forward<Impl>(self)))
      base::_ub(std::forward<Impl>(self));
  }
};
//! [throwing_policy]

//! [outcome_spec]
template <typename T>
using strictOutcome =  //
outcome::basic_outcome<T, std::error_code, std::exception_ptr, abort_policy>;
//! [outcome_spec]

template <typename T, typename EC = std::error_code>
using throwingOutcome =  //
outcome::basic_outcome<T, EC, std::exception_ptr, throwing_policy<T, EC, std::exception_ptr>>;

int main()
{
  try
  {
    throwingOutcome<int> i = std::error_code{};
    i.value();  // throws
    assert(false);
  }
  catch(std::system_error const &)
  {
  }

  strictOutcome<int> i = 1;
  assert(i.value() == 1);
  i.error();  // calls abort()
}
