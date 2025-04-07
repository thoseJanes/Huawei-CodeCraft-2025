#ifndef INDICATOR_H_
#define INDICATOR_H_

#include "global.h"
#ifdef ENABLE_INDICATOR
class Indicator{
public:
    static long int score;
    static long int loss;
    static long int netEarnings;
};

#endif


#endif