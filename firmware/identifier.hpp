#include <Arduino.h>

#ifndef IDENTIFIER_HEADER
#define IDENTIFIER_HEADER

class Identifier {
    public:
        Identifier();

        byte macAddress[6];
        String name;
};

#endif