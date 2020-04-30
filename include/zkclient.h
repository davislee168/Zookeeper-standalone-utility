/*
 * ZooKeeper client wrapper
 *
 */

#ifndef _ZKCLIENT_H
#define _ZKCLIENT_H

#include <list>
#include <unistd.h>
#include <zookeeper.h>
#include "zkconnect.h"
#include "zkexception.h"
#include "zkwatcher.h"

class zkclient: public zkwatcher
{
  public:
#define ZKCLIENT_WAITING_SECS (300)
#define ZKCLIENT_WAITING_CONNECT_SECS (30)
  zkclient(const string &connectingstring, long sessionTimeout, long connectionTimeout);
  ~zkclient();

  static const char* error_string(int32_t rc);

  // close the client
  void close();

  // connect to zookeeper
  void connect(long timeout, zkwatcher *watch);

  string get_current_connected_ip_and_port();

  // create a znode
  string create(const string &path, const char data[], int mode);

  // create an ephemeral znode with empty data
  void createEphemeral(const string &path);

  // create an enphemeral znode
  void createEphemeral(const string &path, const char data[]);

  // create an enphemeral, sequential znode
  string createEphemeralSequential(const string &path, const char data[]);

  // create a persistent znode with empty data
  void createPersistent(const string &path);

  // create a persistent znode with empty data
  void createPersistent(const string &path, bool crteateParents);

  // create a persistent znode
  void createPersistent(const string &path, const char data[]);

  // create a persistent, sequential znode
  string createPersistentSequential(const string &path, const char data[]);

  /** 
   ** create persistent node
   **/
  zoo_rc create_persistent_node(const char* path, const std::string& value, const td::vector<zoo_acl_t>& acl);

  /** 
   ** sequance node's name is 'path-xx' rather than 'path', the xx is auto-increment number
   **
   ** @param returned_path_name - return the real path name of the node
   **/
  zoo_rc create_sequence_node(const char* path, const std::string& value, const std::vector<zoo_acl_t>&acl, std::string& returned_path_name);

  /** 
   ** the ephemeral node will auto delete if the session is disconnect
   **/
  zoo_rc create_ephemeral_node(const char* path, const std::string& value, const std::vector<zoo_acl_t>& acl);

  /** 
   ** @param returned_path_name - return the real path name of the node
   **/
  zoo_rc create_sequance_ephemeral_node(const char* path, const std::string& value, const std::vector<zoo_acl_t>& acl, std::string& returned_path_name);

  zoo_rc exists_node(const char* path, bool watch);

  // delete a znode
  bool deleteZnode(const string &path);
  zoo_rc delete_node(const char* path, int32_t version);
  bool deleteRecursive(const string &path);
  zoo_rc get_node_value(const char* path, string& out_value, zoo_state_t* info, bool watch);
  zoo_rc set_node_value(const char* path, const string& value, int32_t version);

  // check the node exists; return true or false
  bool exists(const string &path);
  bool exists(const string &path, bool watch);
 
  // get the children for the znode
  // return name or null (znode not exists)
  list<string> getChildren(const string &path);
  list<string> getChildren(const string &path, bool watch);

  void watchForData(const string &path);

  // wait for the connected state
  // return true if the client connects the server brfore the time expired
  bool waitUntilConnected(long time);

  // wait time unit until the node exists
  // return true if the node exists
  bool waitUntilExists(const string &path, long timeUnit, long);

  // wait some time for the state
  // return true if the connection state is the keeperState before timeout
  // bool waitForKeeperState(zkconnect::keeperState keeperState, long time, long timeUnit);
  bool waitForKeeperState(zkconnect::keeperState keeperState, long timeout);

  // check connecting status
  bool isConnected();

  /** 
   ** @brief create world acl
   ** @param perms - see {@link #zoo_perm}
   **/
  static zoo_acl_t create_world_acl(int32_t perms);

  void process();

private:
  void reconnect();
  bool deletePath(const string &path);
  bool hasListeners(const string &path);
  zkconnect::keeperState m_currentState;
  std::mutex m_mutex;

protected:
  zkconnect *m_connection;
};
#endif // _ZKCLIENT_H
