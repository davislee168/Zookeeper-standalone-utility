#include "zkclient.h"
#include "zkdlock.h"
#include <iostream>
#include <string>
#include <list>

using namespace std;

int main()
{
  // string hostname = "delfdta9awsc:2177";
  string hostname = "113.140.207.132:2177,113.140.207.133:2177,113.140.207.134:2177";
  string lockpath = "/test1";
  string lockname = "dlock";
  long sessionTimeout = 10000;
  bool watchPrecedingNode = false;

  // get "/lock" to m_dLockPath, assign "dlock" to m_dLockName
  // connect to zookeeper and create persistent "/test1" node
  // zkdlock dlock(hostname);
  // zkdlock dlock(hostname, watchPrecedingNode);
  // zkdlock dlock(hostname, lockname, watchPrecedingNode);
  zkdlock dlock(hostname, lockpath, lockname, watchPrecedingNode);
  // dlock.construct_zkclient(hostname, sessionTimeout, 60);

  // test distributed lock
  while (dlock.dlock()) {
    std::cout << "*** Obtain a key, do OqsManager ***" << std::endl;

    // do process
    while (true) {
      // sleep(300);
      if (dlock.getZkclient() == NULL) {
        std::cout << "Connection broken ..." << std::endl;
        break;
      }
    }
  }

  // disconnect
  // dlock.getZkclient()->close();
  dlock.close();
  std::cout << "disconnected" << std::endl;
  return 0;
}
