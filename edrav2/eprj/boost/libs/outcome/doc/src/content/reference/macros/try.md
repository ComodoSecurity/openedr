+++
title = "`BOOST_OUTCOME_TRY(var, expr)`"
description = "Evaluate an expression which results in a type matching the `ValueOrError<T, E>` concept, assigning `T` to a variable called `var` if successful, immediately returning `try_operation_return_as(expr)` from the calling function if unsuccessful."
+++

Evaluate an expression which results in a type matching the {{% api "ValueOrError<T, E>" %}} concept, assigning `T` to a variable called `var` if successful, immediately returning {{% api "try_operation_return_as(expr)" %}} from the calling function if unsuccessful.

*Overridable*: Not overridable.

*Definition*: See {{% api "BOOST_OUTCOME_TRYV(expr)" %}} for most of the mechanics.

If successful, an `auto &&var` is initialised to the expression result's `.assume_value()` if available, else to its `.value()`. This binds a reference possibly to the `T` stored inside the bound result of the expression, but possibly also to a temporary emitted from the value observer function.

*Header*: `<boost/outcome/try.hpp>`

