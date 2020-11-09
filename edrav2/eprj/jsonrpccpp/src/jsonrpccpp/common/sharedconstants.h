#ifndef SHAREDCONSTANTS_H
#define SHAREDCONSTANTS_H

#ifdef __APPLE__
    #include <sys/syslimits.h>
#endif

#ifdef __linux__
    #include <linux/limits.h>
#endif


#define DEFAULT_DELIMITER_CHAR char(0x0A)
#define DEFAULT_BUFFER_SIZE 1024


#endif // SHAREDCONSTANTS_H
