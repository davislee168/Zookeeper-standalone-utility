*******************************************************
?
zkwatcher.h#ifndef _ZKWATCHER_H
#define _ZKWATCHER_H
?
#include <string>
#include "zkconnect.h"
?
class zkwatcher
{
public:
zkwatcher();
virtual ~zkwatcher();
?
// thread function
void run();
bool start();
?
bool isShutDown();
?
virtual void process() = 0;
};
?
#endif // _ZKWATCHER_H

