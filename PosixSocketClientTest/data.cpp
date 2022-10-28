#include "memory.h"

#include <iostream>
#include <fstream>
#include <math.h>
#include <time.h>

using namespace std;

extern SharedMemoryStruct SM;
extern InvariantMemoryStruct IM;


bool isNowTimeFor(TradingPeriod testPeriod)
{
    time_t dayEnd_t, now_t;
    int period;

    time(&dayEnd_t);

    struct tm *dayEnd = localtime(&dayEnd_t);
    dayEnd->tm_hour = 15;
    dayEnd->tm_min = 0;
    dayEnd->tm_sec = 0;

    //handle incomplete trading days

    dayEnd_t = mktime (dayEnd);

    time(&now_t);

    double diff = difftime (dayEnd_t,now_t);

    if(diff < 60) //last minute
    {
    	period = DUMPING;
    }
    else if(diff < 60*12)
    {
    	period = UNWINDING;
    }
    else
    {
    	period = NORMAL_TRADING;
    }
period = NORMAL_TRADING;

    return (period == testPeriod);
}

