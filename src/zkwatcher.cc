***************************************************
?
zkwatcher.cpp#include "zkwatcher.h"
?
zkwatcher::zkwatcher()
{
start();
}
?
zkwatcher::~zkwatcher()
{
}
?
void zkwatcher::run()
{
while (!isShutDown())
{
try {
process();
} catch (...) {
}
}
}
?
bool zkwatcher::start()
{
?
}
?
bool zkwatcher::isShutDown()
{
?
}

