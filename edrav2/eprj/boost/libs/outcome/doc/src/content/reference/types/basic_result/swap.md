+++
title = "`void swap(basic_result &)`"
description = "Swap one basic_result with another. Noexcept propagating."
categories = ["modifiers"]
weight = 900
+++

Swap one basic_result with another.

*Requires*: Always available.

*Complexity*: Same as for the `swap()` implementations of the `value_type` and `error_type`. The noexcept of underlying operations is propagated.

*Guarantees*: If an exception is thrown during the operation, the state of both operands on entry is guaranteed restored, if at least one of the underlying operations is marked `noexcept`. If neither is marked `noexcept`, an attempt is made to undo the partially completed operation. If that too fails, the flag bits are forced to something consistent such that there can be no simultaneously valued and errored state, or valueless and errorless.
