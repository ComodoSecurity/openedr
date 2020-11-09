cmake_minimum_required(VERSION 2.8)
project(catch_builder CXX)
include(ExternalProject)

ExternalProject_Add(
    catch
    PREFIX ${CMAKE_BINARY_DIR}/catch
    URL https://github.com/catchorg/Catch2/archive/v2.0.1.tar.gz
    URL_HASH SHA1=5c191a031edebd0525640ed2f38cbf64bacb1803
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(catch source_dir)
set(CATCH_INCLUDE_DIR ${source_dir}/single_include CACHE INTERNAL "Path to include folder for Catch")
