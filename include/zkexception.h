******************************************************
?
zkexception.h
#ifndef _ZKEXCEPTION_H
#define _ZKEXCEPTION_H
?
#include "zookeeper.h"
#include <string>
#include <stdexcept>
?
using namespace std;
?
class zkException:public std::out_of_range
{
public:
zkException(string &msg):out_of_range(msg)
{
m_msg = msg;
}
zkException(int code, string &msg):out_of_range(msg)
{
m_errCode = code;
m_msg = msg;
}
virtual ~zkException() throw(){};
?
private:
string m_msg;
int m_errCode;
};
?
class zkExceptionLossConnection: public zkException
{
public:
zkExceptionLossConnection(int code, string &msg):zkException(msg){}
~zkExceptionLossConnection() throw(){};
};
?
class zkExceptionNoNode: public zkException
{
public:
zkExceptionNoNode(int code, string &msg):zkException(msg){}
~zkExceptionNoNode() throw(){};
};
?
class zkExceptionNodeExists: public zkException
{
public:
zkExceptionNodeExists(int code, string &msg):zkException(msg){}
~zkExceptionNodeExists() throw(){};
};
?
class zkExceptionSessionExpired: public zkException
{
public:
zkExceptionSessionExpired(int code, string &msg):zkException(msg){}
~zkExceptionSessionExpired() throw(){};
};
?
#endif // _ZKEXCEPTION_H

