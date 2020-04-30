/*
 * It is a client wrapper which enable client to connect ZooKeeper ensemble, create persistent[sequential]/
 * ephemeral[sequential] znode, delete znode and watch znode.
*/

#include "zkclient.h"
#include "zkconnect.h"
#include "zookeeper.h"

static const int32_t zoo_path_buf_len = 1024;
static const int32_t zoo_value_buf_len = 10240;

zkclient::zkclient(const string &connectstring, long sessionTimeout, long connectionTimeout):zkwatcher()
{
  m_connection = new zkconnect(connectstring, sessionTimeout);
  connect(connectionTimeout, this);
}

zkclient::~zkclient()
{
  if (m_currentState != zkconnect::NOTCONNECTED)
  close();

  delete m_connection;
}

string zkclient::get_current_connected_ip_and_port()
{
  return m_connection->get_current_connected_ip_and_port();
}

const char* zkclient::error_string(int32_t rc)
{
  return zerror(rc);
}

zoo_rc zkclient::create_persistent_node(const char* path, const std::string& value, const 
std::vector<zoo_acl_t>& acl)
{
  return m_connection->create_node(path, value, acl, 0, nullptr, 0);
}

zoo_rc zkclient::create_sequence_node(const char* path, const std::string& value, const 
std::vector<zoo_acl_t>& acl,
std::string& returned_path_name)
{
  char rpath[zoo_path_buf_len] = { 0 };
  zoo_rc rt = m_connection->create_node(path, value, acl, ZOO_SEQUENCE, rpath, (int32_t)sizeof(rpath));
  returned_path_name = rpath;
  return rt;
}

zoo_rc zkclient::create_ephemeral_node(const char* path, const std::string& value, const 
std::vector<zoo_acl_t>& acl)
{
  return m_connection->create_node(path, value, acl, ZOO_EPHEMERAL, nullptr, 0);
}

zoo_rc zkclient::create_sequance_ephemeral_node(const char* path, const std::string& value, const 
std::vector<zoo_acl_t>& acl,
std::string& returned_path_name)
{
  char rpath[zoo_path_buf_len] = { 0 };
  zoo_rc rt = m_connection->create_node(path, value, acl, ZOO_SEQUENCE | ZOO_EPHEMERAL, rpath, 
  (int32_t)sizeof(rpath));
  returned_path_name = rpath;
  return rt;
}

zoo_rc zkclient::exists_node(const char* path, bool watch)
{
  zoo_rc rt = m_connection->exists_node(path, (int)watch);

return rt;
}

zoo_rc zkclient::delete_node(const char* path, int32_t version)
{
  return (zoo_rc)zoo_delete((zhandle_t*) m_connection->get_zhandle(), path, version);
}

bool zkclient::deleteRecursive(const string &path)
{
  list<string> children;
  try {
    children = getChildren(path, false);
  } catch (zkExceptionNoNode &e) {
    return true;
  }

  for (list<string>::iterator iter = children.begin(); iter != children.end(); iter++)
  {
    string subpath = path + "/" + *iter;
    if (!deleteRecursive(subpath)) {
      return false;
    }
  }
  return deletePath(path);
}

zoo_rc zkclient::get_node_value(const char* path, string& out_value, zoo_state_t* info, bool watch)
{
  struct Stat s = { 0 };

  char buf[zoo_value_buf_len] = { 0 };
  int buf_size = sizeof(buf);
  zoo_rc rt = (zoo_rc)zoo_get((zhandle_t*) m_connection->get_zhandle(), path, watch, buf, &buf_size, &s);
  if (rt == z_ok) {
    out_value = std::move(std::string(buf, buf_size));
  }
  return rt;
}

zoo_rc zkclient::set_node_value(const char* path, const string& value, int32_t version)
{
  return (zoo_rc)zoo_set((zhandle_t*)m_connection->get_zhandle(), path, value.c_str(), (int)value.size(), 
  version);
}

void zkclient::createEphemeral(const string &path)
{
  create(path, NULL, ZOO_EPHEMERAL);
}

void zkclient::createEphemeral(const string &path, const char data[])
{
  create(path, data, ZOO_EPHEMERAL);
}

string zkclient::createEphemeralSequential(const string &path, const char data[])
{
  return (create(path, data, ZOO_EPHEMERAL | ZOO_SEQUENCE));
}

void zkclient::createPersistent(const string &path)
{
createPersistent(path, false);
}

void zkclient::createPersistent(const string &path, bool createParents)
{
  create(path, NULL, 0);

if (createParents) {
    string parentDir = path.substr(0, path.find_last_of('/'));
    createPersistent(parentDir, createParents);
    createPersistent(path, createParents);
  }
}

void zkclient::createPersistent(const string &path, const char data[])
{
  create(path, data, 0);
}

string zkclient::createPersistentSequential(const string &path, const char data[])
{
  return (create(path, data, ZOO_SEQUENCE));
}

string zkclient::create(const string &path, const char data[], int mode)
{
  const char *bytes = data;

  while (true) {
    try {
      return(m_connection->create(path, bytes, mode));
    } catch(zkExceptionLossConnection &e) {
      waitUntilConnected(ZKCLIENT_WAITING_CONNECT_SECS);
    } catch(zkExceptionNoNode &e) {
      throw e;
    } catch(zkExceptionNodeExists &e) {
      throw e;
    } catch(zkException &e) {
      throw e;
    }
  }
}

bool zkclient::deleteZnode(const string &path)
{
  list<string> children;

  try {
    children = getChildren(path, false);
  } catch(zkExceptionNoNode &e) {
    return true;
  }
  
  for (list<string>::iterator iter = children.begin(); iter != children.end(); iter++)
  {
    string subpath = path + "/" + *iter;
    if (!deleteZnode(subpath)) {
      return true;
    }
  }
  return deletePath(path);
}

bool zkclient::deletePath(const string &path)
{
  while (true) {
    try {
      m_connection->deleteZnode(path);
      return true;
    } catch(zkExceptionNoNode &e) {
      throw e;
    } catch(zkExceptionLossConnection &e) {
      waitUntilConnected(ZKCLIENT_WAITING_CONNECT_SECS);
    } catch(zkException &e) {
      throw e;
    }
  }
}

bool zkclient::hasListeners(const string &path)
{
  // DLee needs to implement
  return false;
}

list<string> zkclient::getChildren(const string &path)
{
  return getChildren(path, hasListeners(path));
}

list<string>zkclient::getChildren(const string &path, bool watch)
{
  while (true) {
    try {
      return m_connection->getChildren(path, watch);
    } catch(zkExceptionLossConnection &e) {
      waitUntilConnected(ZKCLIENT_WAITING_CONNECT_SECS);
    } catch(zkException &e) {
      throw e;
    }
  }
}

bool zkclient::exists(const string &path, bool watch)
{
  return m_connection->exists(path, watch);
}

bool zkclient::exists(const string &path)
{
  return exists(path, hasListeners(path));
}

void zkclient::connect(long timneout, zkwatcher *watcher)
{
  std::unique_lock<std::mutex> lck(m_mutex);
  try {
    m_connection->connect(watcher);
    lck.unlock();
    m_currentState = zkconnect::CONNECTED;
  } catch(zkExceptionLossConnection &e) {
    lck.unlock();
    throw e;
  }
}

void zkclient::close()
{
  std::unique_lock<std::mutex> lck(m_mutex);
  m_connection->close();
  m_currentState = zkconnect::NOTCONNECTED;
  lck.unlock();
}

void zkclient::reconnect()
{
  std::unique_lock<std::mutex> lck(m_mutex);
  try {
    m_connection->close();
    m_connection->connect(this);
    m_currentState = zkconnect::CONNECTED;
    lck.unlock();
  } catch(zkExceptionLossConnection &e) {
    lck.unlock();
    throw e;
  }
}

bool zkclient::waitUntilExists(const string &path, long timeUnit, long time)
{
  return false;
}

bool zkclient::waitUntilConnected(long time)
{
  return waitForKeeperState(zkconnect::CONNECTED, time);
}

bool zkclient::waitForKeeperState(zkconnect::keeperState keeperState, long timeout)
{
  std::unique_lock<std::mutex> lck(m_mutex);
  bool stillWaiting = true;
  while (m_currentState != keeperState)
  {
    if (!stillWaiting)
    {
      lck.unlock();
      return false;
    }
    // DLee stillWaiting = awaitUntilStateChanged(timeout);
  }
  lck.unlock();
  return true;
}

void zkclient::process()
{
  // DLee
  return;
}

void zkclient::watchForData(const string &path)
{
  while (true) {
    try {
      exists(path, true);
    } catch (zkExceptionNoNode &e) {
      return;
    } catch (zkExceptionLossConnection &e) {
      sleep(ZKCLIENT_WAITING_SECS);
      waitUntilConnected(ZKCLIENT_WAITING_CONNECT_SECS);
    } catch (zkException &e) {
      throw e;
    }
  }
}

zoo_acl_t zkclient::create_world_acl(int32_t perms)
{
  zoo_acl_t acl("world", "anyone", perms);
  return acl;
}

