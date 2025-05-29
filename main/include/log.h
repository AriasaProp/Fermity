/*
 * 
 *
 * File: log.h
 * Author: AriasaProp
 *
 * Description: include messaging macro function for dealing with console message.
 */
 
 
#ifndef LOG_INCLUDED_
#define LOG_INCLUDED_

#include <stdio.h>

#ifdef DEBUG
#define MESSAGE(X) printf(X)
#else
#define MESSAGE(X)
#endif


#endif // LOG_INCLUDED_