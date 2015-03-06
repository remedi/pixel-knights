// This file defines the functions declared in server.h
//
// Authors: Pixel Knights
// Date: 6.3.2015

#include <sys/types.h>
#include <unistd.h>

#include "server.h"
#include "typedefs.h"

// Returns the maximum of a and b
int max(int a, int b) {

    if (a > b)
        return a;
    return b;
}

// Returns the ID of a new client connection
ID createID(void) {

    // TODO: This dummy always returns the same value
    return getpid();
}
