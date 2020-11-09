+++
title = "`try_operation_return_as(expr)`"
description = "A customisable converter of `ValueOrError<T, E>` concept matching types to a returnable failure type."
+++

A customisable converter of {{% api "ValueOrError<T, E>" %}} concept matching types to a returnable failure type.

*Overridable*: Argument dependent lookup.

*Default*: A number of implementations are provided by default:

1. `try_operation_return_as(T &&)` which requires `T` to provide an `.as_failure()` member function in order to be available. This is selected for all `basic_result` and `basic_outcome` types. See {{% api "auto try_operation_return_as(T &&)" %}}.

2. Copy and move editions of `try_operation_return_as(std::experimental::expected<T, E>)` which return a `std::experimental::unexpected<E>` for the input's `.error()` member function. See {{% api "std::experimental::unexpected<E> try_operation_return_as(std::experimental::expected<T, E>)" %}}.

*Namespace*: `BOOST_OUTCOME_V2_NAMESPACE`

*Header*: `<boost/outcome/try.hpp>`
