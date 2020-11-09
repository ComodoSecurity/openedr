+++
title = "`auto try_operation_return_as(T &&)`"
description = "Implementation of `try_operation_return_as(expr)` ADL customisation point for `basic_result` and `basic_outcome`."
+++

This implementation of {{% api "try_operation_return_as(expr)" %}} returns whatever the input type's `.as_failure()` member function returns.
`basic_result` and `basic_outcome` provide such a member function, see {{% api "auto as_failure() const &" %}}.

*Requires*: That the expression `std::declval<T>().as_failure()` is a valid expression.

*Namespace*: `BOOST_OUTCOME_V2_NAMESPACE`

*Header*: `<boost/outcome/try.hpp>`
