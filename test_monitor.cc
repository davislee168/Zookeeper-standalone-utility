#include "zkclient.h"
#include "zkdlock.h"
#include <iostream>
#include <string>
#include <list>

using namespace std;

int main()
{
  string hostname = "127.0.0.1:2181,127.0.0.2:2181,127.0.0.3:2181";
  string lockpath = "/test1";
  string lockname = "dlock";
  long sessionTimeout = 10000;
  bool watchPrecedingNode = false;

  // get "/lock" to m_dLockPath, assign "dlock" to m_dLockName
  // connect to zookeeper and create persistent "/test1" node
  // zkdlock dlock(hostname, lockname);
  zkdlock dlock(hostname, lockpath, lockname, watchPrecedingNode);
  // dlock.construct_zkclient(hostname, sessionTimeout, 60);

  // find a pid which holds a key
  bool monitor = dlock.dlockMonitor();

  // disconnect
  // dlock.getZkclient()->close();
  dlock.close();
  std::cout << "disconnected" << std::endl;
  return 0;
}

