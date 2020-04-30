****************************************************
?
zkconnect.h
?
/*
* This program is to wrap zookeeper C API for general purpose.
*
* It provides three major function as subscribe as followings:
* 1. connect - connect to ZooKeeper ensemble
* 2. create - create a znode (may contain the data associated with it or associate with children)
* 3. exists - check whether a znode exists and its information
*
*/
?
#ifndef _ZKCONNECT_H
#define _ZKCONNECT_H
?
#include <string.h>
#include <iostream>
#include <list>
#include <vector>
#include <mutex>
#include <condition_variable>
?
#include "zookeeper.h"
#include "zkexception.h"
// #include "zk_adaptor.h"
?
typedef void (*log_callback_fn)(const char *message);
?
using namespace std;
?
/* error code */
enum zoo_rc {
z_ok = 0, /*!< Everything is OK */
?
/** System and server-side errors.
** This is never thrown by the server, it shouldn't be used other than
** to indicate a range. Specifically error codes greater than this
** value, but lesser than {@link #api_error}, are system errors. */
z_system_error = -1,
z_runtime_inconsistency = -2, /*!< A runtime inconsistency was found */
z_data_inconsistency = -3, /*!< A data inconsistency was found */
z_connection_loss = -4, /*!< Connection to the server has been lost */
z_marshalling_error = -5, /*!< Error while marshalling or unmarshalling data */
z_unimplemented = -6, /*!< Operation is unimplemented */
z_operation_timeout = -7, /*!< Operation timeout */
z_bad_arguments = -8, /*!< Invalid arguments */
z_invliad_state = -9, /*!< Invliad zhandle state */
?
/** API errors.
** This is never thrown by the server, it shouldn't be used other than
** to indicate a range. Specifically error codes greater than this
** value are API errors (while values less than this indicate a
** {@link #system_error}).
**/
z_api_error = -100,
z_no_node = -101, /*!< Node does not exist */
z_no_auth = -102, /*!< Not authenticated */
z_bad_version = -103, /*!< Version conflict */
z_no_children_for_ephemeral = -108, /*!< Ephemeral nodes may not have children */
z_node_exists = -110, /*!< The node already exists */
z_not_empty = -111, /*!< The node has children */
z_session_expired = -112, /*!< The session has been expired by the server */
z_invalid_callback = -113, /*!< Invalid callback specified */
z_invalid_acl = -114, /*!< Invalid ACL specified */
z_auth_failed = -115, /*!< Client authentication failed */
z_closing = -116, /*!< ZooKeeper is closing */
z_nothing = -117, /*!< (not error) no server responses to process */
z_session_moved = -118 /*!<session moved to another server, so operation is ignored */
};
?
/** permissions(ACL Consts)*/
enum zoo_perm {
zoo_perm_read = 1 << 0,
zoo_perm_write = 1 << 1,
zoo_perm_create = 1 << 2,
zoo_perm_delete = 1 << 3,
zoo_perm_admin = 1 << 4,
zoo_perm_all = 0x1f,
};
?
?
struct zoo_acl_t
{
std::string scheme; // one of { 'world', 'auth', 'digest', 'ip'}
std::string id; // the value type is different case the scheme
int32_t perm; // see {@link #zoo_perm}
?
zoo_acl_t() : perm(0){}
zoo_acl_t(const char* _scheme, const char* _id, int32_t _perm) : scheme(_scheme), id(_id), 
perm(_perm) { }
};
?
/** zoo node info */
struct zoo_state_t {
int64_t ctime; // node create time
int64_t mtime; // node last modify time
int32_t version; // node version
int32_t children_count; // the number of children of the node
};
?
typedef zhandle_t* zookeeper;
?
class zkwatcher;
?
class zkconnect
{
// Do nothing for now
friend void watch_init(zhandle_t *zh, int type, int state, const char *path, void* context);
?
// Do nothing for now
friend void watch_exist(zhandle_t *zh, int type, int state, const char *path, void* context);
?
// Watching the initial pid obtains a key if monitor runs ahead of all processes
friend void watch_getChild(zhandle_t *zh, int type, int state, const char *path, void* context);
?
public:
enum keeperState
{
NOTCONNECTED = 0,
SESSION_EXPIRED,
AUTH_FAILED,
CONNECTING,
ASSOCIATING,
CONNECTED
};
?
zkconnect() {};
// Initialize essential elements for connection
zkconnect(const string &zkServers, int sessionTimeout);
~zkconnect();
?
string get_current_connected_ip_and_port();
string get_host_ip();
?
// connect to ZooKeeper ensemble
void connect(zkwatcher *watcher);
?
// close the connection
void close();
?
zhandle_t* get_zhandle();
?
// creste znode: persistent[sequential], ephemeral[sequential]
string create(const string &path, const char data[], int mode);
zoo_rc create_node(const char* path, const std::string& value, const std::vector<zoo_acl_t>& acl, 
int32_t create_flags,
char *path_buffer, int32_t path_buffer_len);
?
// delete znode
void deleteZnode(const string &path);
zoo_rc delete_node(const char* path, int32_t version);
?
// check whether a node exists; it has a watch function as a simple mechanism for the client
bool exists(const string &path, bool watch);
zoo_rc exists_node(const char* path, bool watch);
?
// get children
list<string> getChildren(const string &path, bool watch);
?
const std::string get_connected_host_and_port();
?
static const char *State2String(int p_iState);
static const char *Type2String(int p_iType);
// static string m_ipAndPort;
?
private:
__inline void checkError(int err, string msg)
{
if (err != ZNODEEXISTS)
cout << err << "Errrrrrrrrrrrrrrrrrrr" << endl;
switch (err)
{
case ZCONNECTIONLOSS:
throw zkExceptionLossConnection(err, msg);
case ZNONODE:
throw zkExceptionNoNode(err, msg);
case ZNODEEXISTS:
throw zkExceptionNodeExists(err,msg);
case ZSESSIONEXPIRED:
throw zkExceptionSessionExpired(err,msg);
case ZINVALIDSTATE:
break;
default:
throw zkException(err, msg);
}
}
?
// Watching any event created, changed and deleted
static void globalWatcher(zhandle_t *p_pZH, int p_iType, int p_iState, const char *p_szPath, void 
*p_pWatcherCtx);
static string m_ipAndPort;
?
zhandle_t* m_zk;
string m_servers;
int m_sessionTimeout;
int m_zkstat;
std::mutex m_mutex;
std::condition_variable m_condVar;
};
?
#endif // _ZKCONNECT_H

