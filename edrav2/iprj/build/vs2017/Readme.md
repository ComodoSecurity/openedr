Ñommon build properties for vs2017 projects (iprj only).
They are represented as a set of props-files.

Props-files are 2 types:
  * props-files which are included by vcxproj-file
  * helper props-files (which are included by props-files of the first type)

## Props-files which are included by vcxproj-file
The iprj directory contains several diferent type of project (e.g. lib, test, exe etc.).
Each type of project has own a subset of props-files (e.g. lib_gen.props and lib.props).

Generally each project should import 3 files:
  * consts.props
  * {type}_gen.props
  * {type}.props

Each props is added into vcxproj-file in specific location of a vcxproj-file.

### consts.props
It contains paths to other components and other constants.
It is included at after import of "Microsoft.Cpp.Default.props".
This the only props-file which is included with a relative path.
All other props-files are included with path is contains macroses from consts.props

### {type}_gen.props and {type}.props
They are contains common properties of projects.

2 props-files are used because some properties must be set before
import of "Microsoft.Cpp.props" and some after it.

Import of {type}_gen.props is inserted after import "consts.props".
Import of {type}.props is inserted just after:
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />

## Helper props-files
  * common_gen.props - common properties for all {type}_gen.props
  * common.props - common properties for all {type}.props
  * common_libs_and_includes.props - static libraries and include directories paths

## Editing of props-files and props-files

### Creation and editing of props-files
Props-files are created and edited manually only.

### Creation of vcxproj-files
The best way to create new vcxproj-file is copying of existing vcxproj-files with the same type.
If a developer create new vcxproj-file from Visual Studio, he should manually add import of props-files in it.

### Editing of vcxproj-files
Vcxproj-files are edited from Visual Studion GUI interface (the properties dialog).
But a developer MUST NOT edit project with "Propery manager".

## Project types
  * lib - c++ static library
  * test - c++ unit test

