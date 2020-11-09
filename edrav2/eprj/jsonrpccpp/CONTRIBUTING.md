# Developer Information

If you plan to contribute to libjson-rpc-cpp, please skim the following information.

## Developer Makefile

The [Makefile](Makefile) contains useful commands that makes development for libjson-rpc-cpp easier.

* `make build`: builds the framework in the default configuration
* `make build-docker`: builds the framework for all distributions in docker
* `make test`: runs all tests
* `make format`: formats all cpp and header files with `clang-format`
* `make check-format`: checks if the sources have been formatted
* `make coverage`: generates a coverage report to `reports/coverage.html`
* `make clean`: remove all build artifacts

## Contributions

Contributions of any kind are always very welcome.
Here are some suggestions:

- Bugreports
- Bugfixes
- Extending documentation (especially doxygen)
- Extending the test coverage
- Simplifying the build system
- Suggestion of new features
- New features:
  - Adding new connectors.
  - Adding new languages to the stubgenerator.

Additionally you can find a wishlist and planned features [here](https://github.com/cinemast/libjson-rpc-cpp/projects/1)

### Guidelines / Conventions

We do not want to prevent you from contributing by having too strict guidelines.
If you have ideas for improvement, just do it your way, rather than doing it not at all.

Anyway here is a list of how we would prefer your contributions:
#### Issues:
  - Use the issue tracker on github to report bugs or improvements.
  - Please avoid sending me mails directly, as this is not visible to others.
  - Please close issues on yourself if you think a problem has been dealt with.

#### Code contributions:
  - Please raise a pull-request against the **develop** branch.
  - If you add features, please keep the test-coverage at 100% line coverage and document them accordingly (doxygen, manpage, etc.).
  - If you fix a bug, please refer the issue in the [commit message](https://help.github.com/articles/closing-issues-via-commit-messages/).
  - Please make sure that the travis-ci build passes (you will get notified if you raise a pull-request).
  - Add yourself to the AUTHORS.md.
  - Document your changes in the CHANGELOG.md
  - Format the code using `make format`
