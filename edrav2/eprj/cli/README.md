# cli

A cross-platform header only C++14 library for interactive command line interfaces (Cisco style)

![demo_local_session](https://user-images.githubusercontent.com/5451767/51046611-d1dadc00-15c6-11e9-8a0d-2c66efc83290.gif)

![demo_telnet_session](https://user-images.githubusercontent.com/5451767/51046612-d1dadc00-15c6-11e9-83c2-beadb3593348.gif)

## Features

* Header only
* Cross-platform (linux and windows)
* Menus and submenus
* Remote sessions (telnet)
* History (navigation with arrow keys)
* Autocompletion (with TAB key)
* Async interface
* Colors

## Dependencies

The library depends on boost asio (to provide telnet server)

## Installation

The library is header-only: it consists entirely of header files
containing templates and inline functions, and require no separately-compiled
library binaries or special treatment when linking.

Extract the archive wherever you want.

Now you must only remember to specify the cli and boost paths when
compiling your source code.

## Compilation of the samples

You can find some examples in the directory "samples".
Each .cpp file corresponds to an executable. You can compile each sample by including
cli and boost header files and linking boost system (and pthread on linux).

In the directory samples you can also find:

* a GNU make file (Makefile)
* a Windows nmake file (makefile.win)
* a Visual Studio solution

You can specify boost library path in the following ways:

### GNU Make

    make CXXFLAGS="-isystem <boost_include>" LDFLAGS="-L<boost_lib>"

example:

    make CXXFLAGS="-isystem /opt/boost_1_66_0/install/x86/include" LDFLAGS="-L/opt/boost_1_66_0/install/x86/lib"

(if you want to use clang instead of gcc, you can set the variable CXX=clang++)

### Windows nmake

Set the environment variable BOOST. Then, from a visual studio console, use the command:

    nmake /f makefile.win

### Visual Studio solution

Set the environment variable BOOST. Then, open the file
`cli/samples/samples.sln`

## License

Distributed under the Boost Software License, Version 1.0.
(See accompanying file [LICENSE.txt](LICENSE.txt) or copy at
<http://www.boost.org/LICENSE_1_0.txt>)

## Contact

Please report issues or questions here:
<http://github.com/daniele77/cli/issues>

You can contact me via twitter at @DPallastrelli

---

## Contributing (We Need Your Help!)

Any feedback from users and stakeholders, even simple questions about
how things work or why they were done a certain way, carries value
and can be used to improve the library.

Even if you just have questions, asking them in issues provides valuable
information that can be used to improve the library - do not hesitate,
no question is insignificant or unimportant!
