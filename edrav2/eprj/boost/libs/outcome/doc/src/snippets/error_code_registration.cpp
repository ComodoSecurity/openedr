//! [error_code_registration]
#include <iostream>
#include <string>        // for string printing
#include <system_error>  // bring in std::error_code et al

// This is the custom error code enum
enum class ConversionErrc
{
  Success     = 0, // 0 should not represent an error
  EmptyString = 1,
  IllegalChar = 2,
  TooLong     = 3,
};

namespace std
{
  // Tell the C++ 11 STL metaprogramming that enum ConversionErrc
  // is registered with the standard error code system
  template <> struct is_error_code_enum<ConversionErrc> : true_type
  {
  };
}

namespace detail
{
  // Define a custom error code category derived from std::error_category
  class ConversionErrc_category : public std::error_category
  {
  public:
    // Return a short descriptive name for the category
    virtual const char *name() const noexcept override final { return "ConversionError"; }
    // Return what each enum means in text
    virtual std::string message(int c) const override final
    {
      switch (static_cast<ConversionErrc>(c))
      {
      case ConversionErrc::Success:
        return "conversion successful";
      case ConversionErrc::EmptyString:
        return "converting empty string";
      case ConversionErrc::IllegalChar:
        return "got non-digit char when converting to a number";
      case ConversionErrc::TooLong:
        return "the number would not fit into memory";
      default:
        return "unknown";
      }
    }
    // OPTIONAL: Allow generic error conditions to be compared to me
    virtual std::error_condition default_error_condition(int c) const noexcept override final
    {
      switch (static_cast<ConversionErrc>(c))
      {
      case ConversionErrc::EmptyString:
        return make_error_condition(std::errc::invalid_argument);
      case ConversionErrc::IllegalChar:
        return make_error_condition(std::errc::invalid_argument);
      case ConversionErrc::TooLong:
        return make_error_condition(std::errc::result_out_of_range);
      default:
        // I have no mapping for this code
        return std::error_condition(c, *this);
      }
    }
  };
}

// Define the linkage for this function to be used by external code.
// This would be the usual __declspec(dllexport) or __declspec(dllimport)
// if we were in a Windows DLL etc. But for this example use a global
// instance but with inline linkage so multiple definitions do not collide.
#define THIS_MODULE_API_DECL extern inline

// Declare a global function returning a static instance of the custom category
THIS_MODULE_API_DECL const detail::ConversionErrc_category &ConversionErrc_category()
{
  static detail::ConversionErrc_category c;
  return c;
}


// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
inline std::error_code make_error_code(ConversionErrc e)
{
  return {static_cast<int>(e), ConversionErrc_category()};
}

int main(void)
{
  // Note that we can now supply ConversionErrc directly to error_code
  std::error_code ec = ConversionErrc::IllegalChar;

  std::cout << "ConversionErrc::IllegalChar is printed by std::error_code as "
    << ec << " with explanatory message " << ec.message() << std::endl;

  // We can compare ConversionErrc containing error codes to generic conditions
  std::cout << "ec is equivalent to std::errc::invalid_argument = "
    << (ec == std::errc::invalid_argument) << std::endl;
  std::cout << "ec is equivalent to std::errc::result_out_of_range = "
    << (ec == std::errc::result_out_of_range) << std::endl;
  return 0;
}
//! [error_code_registration]
