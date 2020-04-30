#ifndef _ZKDLOCK_H
#define _ZKDLOCK_H

#include <iostream>
#include <string>
#include <memory>
#include <list>
#include <mutex>
#include <condition_variable>

using namespace std;

class zkclient;

class zkdlock
{
#define M_SESSIONTIMEOUT (30000)
#define M_CONNECTIONTIMEOUT (30)

public:
  zkdlock(const string &hostname);
  zkdlock(const string &hostname, bool watchPrecedingNode);
  zkdlock(const string &hostname, const string &name, bool watchPrecedingNode);
  zkdlock(const string &hostname, const string &lockpath, const string &name, bool 
  watchPrecedingNode);
  ~zkdlock(){}

  void close();

  void construct_zkclient(const string &connectingstring, long sessionTimeout, long connectionTimeout);

  // connect to zookeeper and create a persistent node "/test1"
  bool init();
  bool isNeedCreate();
  bool dlock();
  bool unDlock();
  bool waitDlock(long timeout);
  string findZnodePid(); // find a pid associate with znode having a key
  bool dlockMonitor();

  bool getDlock();
  bool getDlock(string myZnode);
  std::shared_ptr<zkclient> getZkclient();
  void waitForDlock(string lowestZnode, string myZnode);

  const string& getDLockPath() {return m_servDlockPath;}
  const string& getDLockNode() {return m_servDlockNode;}

private:
  string findMinZnode(); // find a minimun znode
  string findMaxZnode(); // find a maximun znode
  string findPreZnode(); // find a preceding znode
  string m_dLockPath; // eq. "/test1"
  string m_dLockName; // eq. "dlock"
  string m_dLockFullPath; // eq. "/test1/dlock"
  string m_dLockZnodeName; // eq. "/test1/dlock0000000001"

  int m_dLockStats;
  int m_dLockNoCreate;
  int m_dLockCreate;

  const string m_servDlockPath = "/test1";
  const string m_servDlockNode = "dlock";

  list<string> m_dLockList;

  std::shared_ptr<zkclient> m_zkclient;
  mutex m_mutex;
  condition_variable m_condVar;

  string m_zkhosts;
  bool m_watchPrecedingNode;
};
#endif // _ZKDLOCK_H

