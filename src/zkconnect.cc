*****************************************************
?
zkconnect.cpp
/*
* ZooKeeper c binding API wrapper to do connect, close, create, delete, getChildren and watch
*/
?
#include <iostream>
#include <sstream> // std::stringstream
#include <thread>
#include <vector>
#include "zkconnect.h"
#include "zookeeper.h"
#include <netinet/in.h> // ntohs(): convert values between host and network byte order
#include <arpa/inet.h> // inet_ntop(): convert IPv4 and Ipv6 addresses from binary to text form
#include <unistd.h> // gethostname()
#include <netdb.h> // gethostbyname()
// #include <zk_adaptor.h>
// #include <zk_hashtable.h>
// #include <addrvec.h>
// #include <winport.h>
?
using namespace std;
std::mutex lck_mutex;
std::condition_variable lck_condVar;
bool lck_ready = 0;
?
void watch_init(zhandle_t *zh, int type, int state, const char *path, void* context)
{
// Do nothing for now
}
?
void watch_exist(zhandle_t *zh, int type, int state, const char *path, void* context)
{
// Do nothing for now
}
?
void watch_getChild(zhandle_t *zh, int type, int state, const char *path, void* context)
{
lck_ready = true;
lck_condVar.notify_all();
}
?
std::string zkconnect::m_ipAndPort = "";
?
void zkconnect::globalWatcher(zhandle_t *p_pZH, int p_iType, int p_iState, const char *p_szPath, void 
*p_pWatcherCtx)
{
struct sockaddr addr_in;
socklen_t addr_in_len = sizeof(addr_in);
char buf[255];
char addrstr[128];
void *inaddr;
int port;
?
// printf("globalWatcher |%s[%d]|%s[%d]\n", Type2String(p_iType), p_iType, State2String(p_iState), 
p_iState);
?
if (p_iType == ZOO_CREATED_EVENT) {
std::cout << "Event created" << std::endl;
} else if (p_iType == ZOO_DELETED_EVENT || p_iType == ZOO_CHANGED_EVENT) {
std::cout << "Znode deleted/changed --- wakeup all waiting processes" << std::endl;
/*
lck_ready = true;
lck_condVar.notify_all();
*/
if (p_pZH != NULL) {
if (zookeeper_get_connected_host(p_pZH, &addr_in, &addr_in_len) == NULL) {
std::cout << "Event deleted/changed: can NOT get host ..." << std::endl;
} else {
lck_ready = true;
lck_condVar.notify_all();
inaddr = &((struct sockaddr_in *) &addr_in)->sin_addr;
port = ((struct sockaddr_in *) &addr_in)->sin_port;
?
inet_ntop(addr_in.sa_family, inaddr, addrstr, sizeof(addrstr)-1);
snprintf(buf, sizeof(buf), "%s:%d", addrstr, ntohs(port));
m_ipAndPort = std::string(buf);
std::cout << "Event deleted/changed: currently connected to " << buf << std::endl;
}
} else {
std::cout << "Event deleted/changed: no zookeeper handler" << std::endl;
}
} else if (p_iType == ZOO_SESSION_EVENT) {
if (p_pZH != NULL) {
if (zookeeper_get_connected_host(p_pZH, &addr_in, &addr_in_len) == NULL) {
std::cout << "Session event: can NOT get host ..." << std::endl;
} else {
lck_ready = true;
lck_condVar.notify_all();
?
/*
zkconnect zkconn;
string host_ip = zkconn.get_host_ip();
std::cout << "Host IP: " << host_ip << std::endl;
*/
?
inaddr = &((struct sockaddr_in *) &addr_in)->sin_addr;
port = ((struct sockaddr_in *) &addr_in)->sin_port;
?
inet_ntop(addr_in.sa_family, inaddr, addrstr, sizeof(addrstr)-1);
snprintf(buf, sizeof(buf), "%s:%d", addrstr, ntohs(port));
m_ipAndPort = std::string(buf);
std::cout << "Session event: currently connected to " << buf << std::endl;
}
} else {
std::cout << "Session event: no zookeeper handler" << std::endl;
}
} else if (p_iType == ZOO_CHILD_EVENT) {
std::cout << "Child event" << std::endl;
} else {
// ZOO_NOTWATCHING_EVENT
std::cout << "Do nothing ..." << std::endl;
}
}
?
const char *zkconnect::State2String(int p_iState)
{
if (p_iState == 0)
return "CLOSED_STATE";
if (p_iState == ZOO_CONNECTING_STATE)
return "CONNECTING_STATE";
if (p_iState == ZOO_ASSOCIATING_STATE)
return "ASSOCIATING_STATE";
if (p_iState == ZOO_CONNECTED_STATE)
return "CONNECTED_STATE";
if (p_iState == ZOO_EXPIRED_SESSION_STATE)
return "EXPIRED_SESSION_STATE";
if (p_iState == ZOO_AUTH_FAILED_STATE)
return "AUTH_FAILED_STATE";
?
return "INVALID_STATE";
}
?
const char *zkconnect::Type2String(int p_iType)
{
if (p_iType == ZOO_CREATED_EVENT)
return "CREATED_EVENT";
if (p_iType == ZOO_DELETED_EVENT)
return "DELETED_EVENT";
if (p_iType == ZOO_CHANGED_EVENT)
return "CHANGED_EVENT";
if (p_iType == ZOO_CHILD_EVENT)
return "CHILD_EVENT";
if (p_iType == ZOO_SESSION_EVENT)
return "SESSION_EVENT";
if (p_iType == ZOO_NOTWATCHING_EVENT)
return "NOTWATCHING_EVENT";
?
return "UNKNOWN_EVENT_TYPE";
}
?
static void state_to_zoo_state_t(const struct Stat& s, zoo_state_t* state)
{
state->ctime = s.ctime;
state->mtime = s.mtime;
state->version = s.version;
state->children_count = s.numChildren;
}
?
?
// build a zookeeper connection
zkconnect::zkconnect(const string &zkServers, int sessionTimeout)
{
m_zk = NULL;
m_servers = zkServers;
m_sessionTimeout = sessionTimeout;
m_zkstat = 999; // #define NOTCONNECTED_STATE 999
}
?
zkconnect::~zkconnect()
{
m_zk = NULL;
m_servers.clear();
m_sessionTimeout = 0;
m_zkstat = 999;
}
?
string zkconnect::get_current_connected_ip_and_port()
{
return m_ipAndPort;
}
?
string zkconnect::get_host_ip()
{
char hostbuffer[256];
char *ipBuffer;
struct hostent *host_entry;
int hostname;
?
// To retrieve hostname
hostname = gethostname(hostbuffer, sizeof(hostbuffer));
?
// To retrieve host information
host_entry = gethostbyname(hostbuffer);

// To convert an internet network address into ASCII string
ipBuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
?
// printf("Hostname: %s\n", hostbuffer);
?
return (std::string(ipBuffer));
}
?
void zkconnect::connect(zkwatcher *watcher)
{
std::lock_guard<std::mutex> lock(m_mutex);
if (m_zk != NULL) {
// return;
close(); // close first
}
?
// try to connect to ZooKeeper ensemble
// C-API zhandle_t *zookeeper_init(const char *host, watcher_fn watcher, int recv_timeout, const 
clientid_t *clientid,
// void *context, int flags);
m_zk = zookeeper_init(m_servers.c_str(), globalWatcher, m_sessionTimeout, 0, (void*) this, 0);
if (m_zk == NULL) {
string err = "zookeeper_init";
throw zkExceptionLossConnection(0, err);
} else {
std::cout << "Zookeeper connected ..." << std::endl;
}
?
zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
// zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
}
?
zhandle_t* zkconnect::get_zhandle()
{
return m_zk;
}
?
void zkconnect::close()
{
std::lock_guard<std::mutex> lock(m_mutex);
if (m_zk != NULL)
{ 
// C-API int zookeeper_close(zhandle_t *zh);
zookeeper_close(m_zk);
m_zk = NULL;
}
}
?
zoo_rc zkconnect::create_node(const char* path, const string& value, const vector<zoo_acl_t>& acl,
int32_t create_flags, char *path_buffer, int32_t path_buffer_len)
{
struct ACL_vector acl_v;
acl_v.count = (int32_t)acl.size();
struct ACL* acl_list = NULL;
if (acl_v.count > 0) {
acl_list = new struct ACL[acl_v.count];
?
for (int32_t i = 0; i < (int32_t)acl.size(); ++i) {
acl_list[i].perms = acl[i].perm;
acl_list[i].id.scheme = (char*)acl[i].scheme.c_str();
acl_list[i].id.id = (char*)acl[i].id.c_str();
}
}
acl_v.data = acl_list;
?
/*
zoo_rc rt = (zoo_rc)zoo_create(m_zk, path, value.c_str(), (int)value.size(), &acl_v, create_flags,
path_buffer, (int)path_buffer_len);
*/
?
// C-API int zoo_create(zhandle_t *zh, const char *path, const char *value, int valuelen, const struct 
acl_vector *acl,
// int mode, char *path_buffer, int path_buffer_len);
?
zoo_rc rt; 
if (value.empty())
rt = (zoo_rc)zoo_create(m_zk, path, NULL, 0, &acl_v, create_flags, path_buffer, (int)path_buffer_len);
else
rt = (zoo_rc)zoo_create(m_zk, path, value.c_str(), (int)value.size(), &acl_v, create_flags, path_buffer, 
(int)path_buffer_len);
?
if (acl_list != NULL) {
delete[]acl_list;
}
?
/*
if (rt != z_ok) {
string err = "zoo_create" + std::string(path);
printf("[%s.%d] rt=%d buffer = %s\n", __FUNCTION__, __LINE__, rt, path_buffer);
checkError(rt, err);
}
*/
?
return rt;
}
?
zoo_rc zkconnect::delete_node(const char* path, int32_t version)
{
// C_API int zoo_delete(zhandle_t *zh, const char *path, int version);
return (zoo_rc)zoo_delete(m_zk, path, version);
?
/*
zoo_rc rd = (zoo_rc)zoo_delete(m_zk, path, version); // -1 : not check version
if (rd != z_ok)
{
string err = "zoo_delete" + std::string(path);
printf("[%s.%d\n", __FUNCTION__, __LINE__);
checkError(rd, err);
}
?
return rd;
*/
}
?
zoo_rc zkconnect::exists_node(const char* path, bool watch)
{
// zoo_state_t *info;
struct Stat s = { 0 };
zoo_rc rt;
?
// C-API int zoo_exists(zhandle_t *zh, const char *path, int watch, Struct Stat *stat);
rt = (zoo_rc)zoo_exists(m_zk, path, (int)watch, &s);
?
/*
if (rt != z_ok)
{
string err = "zoo_exists" + std::string(path);
printf("[%s.%d] [rt = %d err = %s]\n", __FUNCTION__, __LINE__, rt, err.c_str());
checkError(rt, err);
}
*/
?
// if (info) {
// state_to_zoo_state_t(s, info);
// }
?
return rt;
}
?
string zkconnect::create(const string &path, const char data[], int mode)
{
string buf;
char buffer[512] = {0};
int rc = ZOK;
?
std::cout << "path: " << path << " data: " << data << " mode: " << mode << std::endl;
// C-API int zoo_create(zhandle_t *zh, const char *path, const char *value, int valuelen, const struct 
acl_vector *acl,
// int mode, char *path_buffer, int path_buffer_len);
if (data == NULL)
rc = zoo_create(m_zk, path.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, mode, buffer, sizeof(buffer)-1);
else
rc = zoo_create(m_zk, path.c_str(), data, sizeof(data)-1, &ZOO_OPEN_ACL_UNSAFE, mode, buffer, 
sizeof(buffer)-1);
?
// zoo_rc rt = (zoo_rc)zoo_create(m_zk, path.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, mode, buffer, 
sizeof(buffer)-1);
?
/*
if (rc == ZNODEEXISTS) {
std::cout << "Nodes already exists: " << path << std::endl;
buf = path;
return buf;
}
*/
?
if (rc != ZOK) {
string err = "zoo_create" + path;
printf("[%s.%d] rc=%d buffer = %s\n", __FUNCTION__, __LINE__, rc, buffer);
checkError(rc, err);
}
?
buf = buffer;
return buf;
}
?
void zkconnect::deleteZnode(const string &path)
{
if (m_zk == NULL)
return;
?
// C_API int zoo_delete(zhandle_t *zh, const char *path, int version);
int rd = zoo_delete(m_zk, path.c_str(), -1); // -1 : not check version
if (rd != ZOK)
{
string err = "zoo_delete" + path;
printf("[%s.%d\n", __FUNCTION__, __LINE__);
checkError(rd, err);
}
}
?
bool zkconnect::exists(const string &path, bool watch)
{
int ret = 0;
if (m_zk == NULL)
return false;
?
if (watch)
{
// C-API int zoo_wexists(zhandle_t *zh, const char *path, watcher_fn watcher, void* watcherCtx, Struct 
Stat *stat);
ret = zoo_wexists(m_zk, path.c_str(), watch_exist, (void*) this, NULL);
if (ret != ZOK)
{
string err = "zoo_wexists" + path;
printf("[%s.%d\n", __FUNCTION__, __LINE__);
checkError(ret, err);
}
return true;
}
?
// C-API int zoo_exists(zhandle_t *zh, const char *path, int watch, Struct Stat *stat);
ret = zoo_exists(m_zk, path.c_str(), 0, NULL);
if (ret != ZOK)
{
string err = "zoo_exists" + path;
printf("[%s.%d] [ret = %d err = %s]\n", __FUNCTION__, __LINE__, ret, err.c_str());
checkError(ret, err);
}
return true;
}
?
list<string> zkconnect::getChildren(const string &path, bool watch)
{
int i = 0;
int ret = 0;
list<string> children;
struct String_vector str_vec;
?
if (watch)
{
// C-API int zoo_wget_children(xhandle_t *zh, const char *path, watcher_fn watcher, void *watcherCtx,
// struct String_vector *strings);
ret = zoo_wget_children(m_zk, path.c_str(), watch_getChild, (void*) this, &str_vec);
if (ret != ZOK)
{
string err = "zoo_wget_children" + path;
// printf("[%s.%d]\n", __FUNCTION__, __LINE__);
checkError(ret, err);
}
}
?
// C-API int zoo_get_children(zhandle_t *zh, const char *path, int watch, struct String_vector *strings);
ret = zoo_get_children(m_zk, path.c_str(), 0, &str_vec);
if (ret != ZOK)
{
string err = "zoo_get_children" + path;
checkError(ret, err);
}
children.clear();
?
std::unique_lock<std::mutex> dlock(m_mutex);
for (i = 0; i < str_vec.count; i++)
{
children.push_back(str_vec.data[i]);
}
dlock.unlock();
?
return children;
}
?
const std::string zkconnect::get_connected_host_and_port()
{ 
std::stringstream ret;
struct sockaddr_in6 addr_in6;
socklen_t addr_len = sizeof(addr_in6);
?
#ifdef _SKIP_1
struct sockaddr addr_in;
socklen_t addr_in_len = sizeof(addr_in);
if (zookeeper_get_connected_host(m_zk, &addr_in, &addr_in_len) == NULL) {
printf("Trow exception ...\n");
} else {
printf("SA_FAMILY = %d\n", addr_in.sa_family);
?
char buf[255];
char addrstr[128];
void *inaddr;
int port;
?
#if defined(AF_INET6)
if (addr_in.sa_family == AF_INET6) {
inaddr = &((struct sockaddr_in6 *) &addr_in)->sin6_addr;
port = ((struct sockaddr_in6 *) &addr_in)->sin6_port;
} else {
#endif
?
inaddr = &((struct sockaddr_in *) &addr_in)->sin_addr;
port = ((struct sockaddr_in *) &addr_in)->sin_port;
#if defined(AF_INET6)
}
#endif
inet_ntop(addr_in.sa_family, inaddr, addrstr, sizeof(addrstr)-1);
snprintf(buf, sizeof(buf), "%s:%d", addrstr, ntohs(port));
?
printf("IP and Port: %s\n", buf);
}
#endif // _SKIP_1
?
/*
#ifdef _SKIP_2
*/
// This return NULL when state is not ZOO_CONNECTED_STATE
if (zookeeper_get_connected_host(m_zk, reinterpret_cast<struct sockaddr*>(&addr_in6), &addr_len) 
== NULL) {
printf("Trow exception\n");
}
?
const struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&addr_in6);
// AF_UNSPEC: 0; AF_INET: 2; AF_INET6: 10
?
if (addr->sa_family == AF_INET) {
// IPv4
printf("IPv4 sa_family = %d\n", addr->sa_family);
const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(addr);
char host[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &(addr_in->sin_addr), host, sizeof(host));
ret << host << ":" << ntohs(addr_in->sin_port);
} else if (addr->sa_family == AF_INET6) {
// IPv6
printf("IPv6 sa_family = %d\n", addr->sa_family);
char host[INET6_ADDRSTRLEN] = {0}; // INET6_ADDRSTRLEN is defined as 46 for IPv6 and 16 for IPv4 in C
// inet_ntop(sock_addr.sa_family, &(sock_addr_in->sin_addr), host, sizeof(host)); 
inet_ntop(AF_INET6, &(addr_in6.sin6_addr), host, sizeof(host)); 
ret << host << ":" << ntohs(addr_in6.sin6_port);
} else {
printf("AF_UNSPEC sa_family = %d\n", addr->sa_family);
// throw exception
}
/*
#endif // _SKIP_2
*/
?
return ret.str();
} 

