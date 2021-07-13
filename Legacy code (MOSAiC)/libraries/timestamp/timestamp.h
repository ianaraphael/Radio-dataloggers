/*
 * timestamp.h
 *
 * Library functions for getting a filename char array built out of RTC
 * timestamp bits
 */

#ifndef timestamp_h
#define timestamp_h

#include <RTCZero.h>

char* getFilename(RTCZero rtc, char *fileName);

#endif // timestamp_h
