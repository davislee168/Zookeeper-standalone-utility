#include "zkdlock.h"
#include "zkclient.h"
#include "zkexception.h"
#include <list>
#include <vector>
#include <sstream>
#include <algorithm>
#include <thread>

using namespace std;

bool ready = false;
extern std::mutex lck_mutex;
extern std::condition_variable lck_condVar;
extern bool lck_ready;

#define _MIN_ZNODE

// const string m_servDlockPath = "/lock";

bool zkdlock::init()
{
  std::vector<zoo_acl_t> acl;
  string value;
  zoo_rc ret = z_ok;

  try {
    if (m_zkclient == NULL) {
      m_zkclient = shared_ptr<zkclient>(new zkclient(m_zkhosts, M_SESSIONTIMEOUT, M_CONNECTIONTIMEOUT));
      if (m_zkclient == NULL) {
        cout << "m_zkclient(new zkclient) err" << endl;
        return false;
      }
      acl.push_back(m_zkclient->create_world_acl(zoo_perm_all));

      // exist_node function returns 0 means node exists
      ret = m_zkclient->exists_node(m_servDlockPath.c_str(), true);
      printf("Try check path[%s] exist[%d], ret[%d][%s]\n", m_servDlockPath.c_str(), ret == z_ok, ret, m_zkclient->error_string(ret));
      if (ret == z_ok) {
        printf("path[%s] already exists, no need to create\n", m_servDlockPath.c_str());
      } else {
        printf("Need to create node\n");
        ret = m_zkclient->create_persistent_node(m_servDlockPath.c_str(), value, acl);
      }

    // m_zkclient->createPersistent(m_servDlockPath, true);
    }
    return true;
  } catch (zkExceptionLossConnection &e) {
    throw e;
  } catch (zkException &e) {
    throw e;
  }
  return false;
}

zkdlock::zkdlock(const string &hostname)
{
  m_zkhosts = hostname;
  m_dLockPath = getDLockPath(); // DLee
  m_dLockName = getDLockNode();
  m_dLockNoCreate = 1;
  m_dLockCreate = 0;
  m_dLockFullPath.clear();
  m_dLockZnodeName.clear();
  m_dLockList.clear();
  m_dLockStats = 1;
  m_watchPrecedingNode = false;
  init(); // DLee
}

zkdlock::zkdlock(const string &hostname, bool watchPrecedingNode)
{
  m_zkhosts = hostname;
  m_dLockPath = getDLockPath(); // DLee
  m_dLockName = getDLockNode();
  m_dLockNoCreate = 1;
  m_dLockCreate = 0;
  m_dLockFullPath.clear();
  m_dLockZnodeName.clear();
  m_dLockList.clear();
  m_dLockStats = 1;
  m_watchPrecedingNode = watchPrecedingNode;
  init(); // DLee
}

zkdlock::zkdlock(const string &hostname, const string &name, bool watchPrecedingNode)
{
  m_zkhosts = hostname;
  m_dLockPath = getDLockPath(); // DLee
  m_dLockName = name;
  m_dLockNoCreate = 1;
  m_dLockCreate = 0;
  m_dLockFullPath.clear();
  m_dLockZnodeName.clear();
  m_dLockList.clear();
  m_dLockStats = 1;
  m_watchPrecedingNode = watchPrecedingNode;
  init(); // DLee
}

zkdlock::zkdlock(const string &hostname, const string &lockpath, const string &name, bool watchPrecedingNode)
{
  m_zkhosts = hostname;
  m_dLockPath = lockpath; // DLee
  m_dLockName = name;
  m_dLockNoCreate = 1;
  m_dLockCreate = 0;
  m_dLockFullPath.clear();
  m_dLockZnodeName.clear();
  m_dLockList.clear();
  m_dLockStats = 1;
  m_watchPrecedingNode = watchPrecedingNode;
  init(); // DLee
}

bool zkdlock::isNeedCreate()
{
  zoo_rc ret = z_ok;
  m_dLockFullPath = m_dLockPath + "/" + m_dLockName;
  ret = m_zkclient->exists_node(m_dLockPath.c_str(), true);
  if (ret == z_ok) {
    printf("path[%s] already exists, no need to create\n", m_dLockPath.c_str());
    m_dLockCreate = 1;
    return false;
  } else {
    printf("Need to create node\n");
    m_dLockCreate = 0;
    return true;
  }

  /*
  bool rc = false;
  try {
    rc = m_zkclient->exists(m_dLockFullPath, false);
    if (rc)
    {
      m_dLockNoCreate = 1;
      return true;
    }
  } catch (zkExceptionNoNode &e) {
    return false;
  }
  return false;
  */
}

string zkdlock::findMinZnode()
{
  string minZnode;

  minZnode.clear();
  m_dLockList.sort();
  if (!m_dLockList.empty())
  {
    minZnode = m_dLockPath + "/" + m_dLockList.front();
  }
  return minZnode;
}

string zkdlock::findMaxZnode()
{
  string maxZnode;

  maxZnode.clear();
  m_dLockList.sort();
  if (!m_dLockList.empty())
  {
    maxZnode = m_dLockPath + "/" + m_dLockList.back();
  }
  return maxZnode;
}

string zkdlock::findPreZnode()
{
  string cur;
  string preZnode;
  string tmp;
  list <string>::iterator iter;

  m_dLockList.sort();

  // Find the preceding lock
  for (iter = m_dLockList.begin(); iter != m_dLockList.end(); ++iter)
  {
    cur = (*iter);
    tmp = m_dLockPath + "/" + cur;
    if (tmp == m_dLockZnodeName)
    {
      if (iter == m_dLockList.begin())
      {
        preZnode = m_dLockZnodeName;
        break;
      }
      --iter;
      cur = (*iter);
      preZnode = m_dLockPath + "/" + cur;
      break;
    }
  }
  return preZnode;
}

string zkdlock::findZnodePid()
{
  zoo_rc ret = z_ok;
  string cur;
  string path;
  string value;
  list <string>::iterator iter;

  // List all children (znode)
  m_dLockList = m_zkclient->getChildren(m_dLockPath, false);

  // Find the value associated with key znode
  for (iter = m_dLockList.begin(); iter != m_dLockList.end(); ++iter)
  {
    cur = (*iter);
    path = m_dLockPath + "/" + cur;
    ret = m_zkclient->get_node_value(path.c_str(), value, nullptr, true);
    // printf("path[%s] value[%s]\n", path.c_str(), value.c_str());

    if (!value.empty()) {
      return value;
    }
  }
  return value;
}

bool zkdlock::dlockMonitor()
{
  zoo_rc ret = z_ok;
  std::vector<zoo_acl_t> acl;
  string value;

  if (!m_dLockCreate)
  {
    if (isNeedCreate())
    {
      // m_zkclient->createPersistent(m_dLockFullPath, true);
      ret = m_zkclient->create_persistent_node(m_servDlockPath.c_str(), value, acl);
    }
  }

  while (true) {
    lck_ready = false;
    // find a pid which holds a key
    string pid = findZnodePid();

    if (!pid.empty()) {
      std::stringstream ss(pid);
      std::vector<string> vect;

      while (ss.good()) {
        string substr;
        getline(ss, substr, '|');
        vect.push_back(substr);
      }
      // std::cout << "PID: " << pid << " holds a key" << vect[0] << " " << vect[1] << std::endl;
      std::cout << "PID: " << vect[0] << " holds a key and connected to " << vect[1] << std::endl;
    } else {
      std::cout << "No process holds a key then monitoring ..." << std::endl;

      list<string> child = m_zkclient->getChildren(m_dLockPath, true);
    }
    std::unique_lock<std::mutex> lck(m_mutex);
    lck_condVar.wait(lck, []{ return lck_ready;}); 
    lck.unlock();
  }

  return true;
}

bool zkdlock::dlock()
{
  zoo_rc ret = z_ok;
  std::vector<zoo_acl_t> acl;
  string value;
  string rpath;

  if (!m_dLockCreate)
  {
    if (isNeedCreate())
    {
      // m_zkclient->createPersistent(m_dLockFullPath, true);
      ret = m_zkclient->create_persistent_node(m_servDlockPath.c_str(), value, acl);
    }
  }

  string lockPath = m_dLockPath + "/" + m_dLockName;
  // m_dLockZnodeName = m_zkclient->createEphemeralSequential(lockPath, NULL);
  acl.push_back(m_zkclient->create_world_acl(zoo_perm_all));
  ret = m_zkclient->create_sequance_ephemeral_node(lockPath.c_str(), value, acl, rpath);
  printf("create sequence ephemeral epath[%s] ret[%d][%s], rpath[%s]\n", lockPath.c_str(), ret, m_zkclient->error_string(ret), rpath.c_str());
  m_dLockZnodeName = rpath;

  m_dLockList = m_zkclient->getChildren(m_dLockPath, false);

  string preZnode;
  if (m_watchPrecedingNode)
  {
    preZnode = findPreZnode();
    std::cout << "My lock is: " << m_dLockZnodeName << " preceding znode is: " << preZnode << std::endl;
  } else {
    preZnode = findMinZnode();
    std::cout << "My lock is: " << m_dLockZnodeName << " min znode is: " << preZnode << std::endl;
  }
  
  if (preZnode == m_dLockZnodeName)
  {
    std::lock_guard<std::mutex> lck(m_mutex);
    // this process obtains a lock then stores the process id to the znode; otherwise does nothing
    string value = std::to_string(getpid());
    string ipAndPort = m_zkclient->get_current_connected_ip_and_port();
    ret = m_zkclient->set_node_value(m_dLockZnodeName.c_str(), value+"|"+ipAndPort, -1);
    // printf("try set path[%s]'s value to [%s] ret[%d][%s]\n", m_dLockZnodeName.c_str(), value.c_str(), ret, m_zkclient->error_string(ret));
    /*
    string pidPath = m_dLockPath + "/" + m_dLockName + std::to_string(getpid());
    ret = m_zkclient->create_ephemeral_node(pidPath.c_str(), value, acl);
    printf("create ephemeral epath[%s] ret[%d][%s]\n", pidPath.c_str(), ret, m_zkclient->error_string(ret));
    */
    return true;
  }

  // waiting for dlock
  while (true) {
    lck_ready = false;
    // Checking preZnode exists or not?
    ret = m_zkclient->exists_node(preZnode.c_str(), true);
    if (ret == z_ok) {
      std::unique_lock<std::mutex> lck(m_mutex);
      
      if (m_watchPrecedingNode)
      {
        std::cout << "The preceding znode: " << preZnode << " still exists, waiting ..." << std::endl;
      } else {
        std::cout << "The min znode: " << preZnode << " still exists, waiting ..." << std::endl;
      }
 
      lck_condVar.wait(lck, []{ return lck_ready;}); 
      // sleep(60);
      lck.unlock();
    } else {
      // try to get dlock
      std::lock_guard<std::mutex> lck(m_mutex);
      std::cout << "preceding znode not exists, try to get dlock" << std::endl;
      m_dLockList = m_zkclient->getChildren(m_dLockPath, false);

      for (auto it = m_dLockList.begin(); it != m_dLockList.end(); it++)
      {
        std::cout << "child is " << (*it) << std::endl;
      }
      
      if (m_watchPrecedingNode)
      {
        preZnode = findPreZnode();
        std::cout << "preceding znode: " << preZnode << std::endl;
      } else {
        preZnode = findMinZnode();
        std::cout << "min znode: " << preZnode << std::endl;
      }

      if (preZnode == m_dLockZnodeName)
      {
        string value = std::to_string(getpid());
        string ipAndPort = m_zkclient->get_current_connected_ip_and_port();
        ret = m_zkclient->set_node_value(m_dLockZnodeName.c_str(), value+"|"+ipAndPort, -1);
        // printf("try set path[%s]'s value to [%s] ret[%d][%s]\n", m_dLockZnodeName.c_str(), value.c_str(), ret, m_zkclient->error_string(ret));
        return true;
      }
    }
  }
}

bool zkdlock::unDlock()
{
  m_zkclient->deleteZnode(m_dLockZnodeName);
}

bool zkdlock::waitDlock(long timeout)
{
  if (!m_dLockNoCreate)
  {
    if (!isNeedCreate())
    {
      m_zkclient->createPersistent(m_dLockFullPath, true);
    }
  }
  string lockPath = m_dLockFullPath + "/";
  m_dLockZnodeName = m_zkclient->createEphemeralSequential(lockPath, NULL);
  m_dLockList = m_zkclient->getChildren(m_dLockFullPath, false);
 
  string minlock = findPreZnode();
  if (minlock == m_dLockZnodeName)
  {
    return true;
  }
}

bool zkdlock::getDlock()
{
  if (!m_dLockNoCreate)
  {
    if (!isNeedCreate())
    {
      m_zkclient->createPersistent(m_dLockFullPath, true);
    }
  }
  string lockPath = m_dLockFullPath + "/";
  m_dLockZnodeName = m_zkclient->createEphemeralSequential(lockPath, NULL);
  
  if (getDlock(m_dLockZnodeName)) {
    return true;
  }
}


bool zkdlock::getDlock(string myZnode)
{
  m_dLockList = m_zkclient->getChildren(m_dLockFullPath, false);
  string nodes[m_dLockList.size()];
  int k = 0;
  for (auto const &i: m_dLockList) {
    nodes[k++] = i;
  }
 
  int n = sizeof(nodes)/sizeof(nodes[0]);
  sort(nodes, nodes+n);
 
  if (myZnode.compare(m_dLockFullPath+"/"+nodes[0]) == 0) {
    // doAction();
    return true;
  }

  waitForDlock(nodes[0], myZnode);

  return true;
}

void zkdlock::construct_zkclient(const string &connectingstring, long sessionTimeout, long connectionTimeout)
{
  // zkclient* zkclientPtr = new zkclient(connectingstring, sessionTimeout, connectionTimeout);
  // m_zkclient.reset(zkclientPtr); 
  m_zkclient = shared_ptr<zkclient>(new zkclient(connectingstring, sessionTimeout, connectionTimeout));
}

void zkdlock::waitForDlock(string lowestZnode, string myZnode)
{
  while (m_zkclient->exists(m_dLockFullPath+"/"+lowestZnode, true)) {
    sleep(60);
  }

  getDlock(myZnode);
}

void zkdlock::close()
{
  m_zkclient->close();
}

std::shared_ptr<zkclient> zkdlock::getZkclient()
{
  return m_zkclient;
}

