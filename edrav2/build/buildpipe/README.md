# buildpipe
Building Pipeline 

## Overview
Building Pipeline is intended to provide automated way to build binaries of EDR Agent v2 (EDRAv2). It supports build of cpecified specified source branch or git-label, performs run of unit tests and uploading of artifacts into common storage.

## Requirements
  * **Git** must be installed and configured
  * **Git-LFS** must be installed
  * **Visual Studio 2017** must be installed and configured
  * **WIX Toolset** must be installed

## Launching
You can launch buildpipe in a different ways:
  * _builder.cmd_ (start without parameters) - will be awaits of user input: branch name or git-label must be provided
  * _builder.cmd release-2.0.0-alpha3_ - building of cpecified branch _release-2.0.0-alpha3_
  * _builder.cmd v.2.0.0-alpha2_ - building of cpecified git-tag _v.2.0.0-alpha2_

## Algorithm of script's work
  * Perform initial checking:
    - if already working one instance of script (exist file _.buildinprocess_) scripts will end it's work
    - if previous run of buildpipe script was ended with error (exist file _.error_) script will ask user what to do next: nothing (exit) or cleanup and continue
    - if branch\tag name was not specified as a commandline, user input is requiered
  * Download source files from Git:
    - _git init_
    - _git clone -n %git\_url% --_
    - _git checkout %branch%_
  * Configures build variables:
    - calculates buld number (+1 to previous build number)
    - generates minimal and full version info
    - generates extra version info
    - generates buildinfo for setup file (_BuildInfo.wxi_)
  * Performs build (compilation) of binarie files
  * Performs build (compilation) of setup files
  * Performs packing distributive files into archive
  * Performs run of unit-tests
  * Uploads artifacts (archives with binarie files, setp files, archive with log-files) to exteral storage (currently SFTP)
  * Performs cleanup operation

At build start, build finish and in case of errors email notification will be sended to specified persons. Short message and archive with log-files is attached into email.

## Error handling
In case of error on any stage:
  * file _.error_ is created
  * email notification with error description and log-files (in archive) is sended to specified persons
  * script ends it's wor with error
In case of error cleanup operation is not performed, so it is possible to investigate the reason of problem.