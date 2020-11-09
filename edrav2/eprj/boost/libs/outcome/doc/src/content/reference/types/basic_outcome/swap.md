+++
title = "`void swap(basic_outcome &)`"
description = "Swap one basic_outcome with another. Noexcept propagating."
categories = ["modifiers"]
weight = 900
+++

Swap one basic_outcome with another.

*Requires*: Always available.

*Complexity*: Same as for the `swap()` implementations of the `value_type`, `error_type` and `exception_type`. The noexcept of underlying operations is propagated.

*Guarantees*: If an exception is thrown during the operation, the state of all three operands on entry is guaranteed restored, if at least two of the underlying operations is marked `noexcept`. If one or zero is marked `noexcept`, an attempt is made to undo the partially completed operation. If that too fails, the flag bits are forced to something consistent such that there can be no simultaneously valued and errored/excepted state, or valueless and errorless/exceptionless.
