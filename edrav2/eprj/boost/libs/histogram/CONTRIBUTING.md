# Contributing to Boost.Histogram

## Star the project

If you like Boost.Histogram, please star the project on Github! We want Boost.Histogram to be the best histogram library out there. If you give it a star, it becomes more visible and will gain more users. More users mean more user feedback to make the library even better.

## Reporting Issues

We value your feedback about issues you encounter. The more information you provide the easier it is for developers to resolve the problem.

Issues should be reported to the [issue tracker](
https://github.com/boostorg/histogram/issues?state=open).

Issues can also be used to submit feature requests.

Don't be shy: if you are friendly, we are friendly! And we care, issues are usually answered within a working day.

## Submitting Pull Requests

Base your changes on `develop`. The `master` branch is only used for releases.

Please rebase your changes on the current develop branch before submitting (which may have diverged from your fork in the meantime).

## Coding Style

Use `clang-format -style=file`, which should pick up the `.clang-format` file of the project. We follow the naming conventions of the C++ standard library. Names start with a small letter, except for template parameters which are capitalized.

## Running Tests

To run the tests, `cd` into the `test` folder and execute `b2`. You can also test the examples by executing `b2` in the `examples` folder.

Please report any tests failures to the issue tracker along with the test
output and information on your system.

## Support

Feel free to send an email to hans.dembinski@gmail.com with any problems or questions.
