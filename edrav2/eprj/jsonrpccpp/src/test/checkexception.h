/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    checkexception.h
 * @date    6/7/2015
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef CHECKEXCEPTION_H
#define CHECKEXCEPTION_H

#define CHECK_EXCEPTION_TYPE(throwCode, exceptionType, expression) {bool thrown = false; try {throwCode;} catch(exceptionType &ex) { CHECK(expression(ex)); thrown = true; } CHECK(thrown);}

#endif // CHECKEXCEPTION_H
