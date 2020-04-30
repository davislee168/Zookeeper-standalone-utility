#include "zkclient.h"
#include "zkdlock.h"
#include <iostream>
#include <string>
#include <vector>
#include <list>

using namespace std;

int main()
{
  string hostname = "127.0.0.1:2181,127.0.0.2:2181,127.0.0.3:2181";
  string path = "/test1";
  string epath = "/test1/dlock";

  string rpath = path;
  string value;
  string cur;
  zoo_rc ret = z_ok;
  long sessionTimeout = 10000;

  // connect to ZooKeeper ensemble
  zkclient cli(hostname, sessionTimeout, 0);

  // test C API functions
  try {
    std::vector<zoo_acl_t> acl;
    acl.push_back(cli.create_world_acl(zoo_perm_all));

    std::cout << "create persistent: " << path << std::endl;
    ret = cli.create_persistent_node(path.c_str(), value, acl);
    printf("create path[%s] ret[%d][%s], rpath[%s]\n", path.c_str(), ret, cli.error_string(ret), rpath.c_str()); 

    // exist_node function returns 0 means node exists
    ret = cli.exists_node(path.c_str(), true);
    if (ret == z_ok) {
      printf("path[%s] already exists, no need to create\n", path.c_str());
    } else {
      printf("Need to create node\n");
    }
    printf("try check path[%s] exist[%d], ret[%d][%s]\n", path.c_str(), ret == z_ok, ret, cli.error_string(ret));

    rpath = epath;

    ret = cli.exists_node(epath.c_str(), true);
    if (ret == z_no_node) {
      ret = cli.create_ephemeral_node(epath.c_str(), value, acl);
      printf("create ephemeral epath[%s] ret[%d][%s], rpath[%s]\n", epath.c_str(), ret, cli.error_string(ret), rpath.c_str()); 
    } else {
      printf("exist ephemeral epath[%s] ret[%d][%s]\n", epath.c_str(), ret, cli.error_string(ret)); 
    }

    ret = cli.create_sequance_ephemeral_node(epath.c_str(), value, acl, rpath);
    printf("create sequence ephemeral epath[%s] ret[%d][%s], rpath[%s]\n", epath.c_str(), ret, cli.error_string(ret), rpath.c_str());

    string nodes[10];
    for (int i = 0; i < 10; i++) {
      ret = cli.create_sequance_ephemeral_node(epath.c_str(), value, acl, rpath);
      nodes[i] = rpath;
    }

    list<string> child = cli.getChildren(path, true);
    list<string>::iterator it;
    for (it = child.begin(); it != child.end(); it++)
    {
      cout << "my child is " << (*it) << endl;
    }

    cli.delete_node(epath.c_str(), -1);

    string ss = epath + std::to_string(getpid());
    printf("pid string = %s\n", ss.c_str());
    ret = cli.create_ephemeral_node(ss.c_str(), value, acl);

    cout << "After delete node [epath] " << epath << endl;
    child = cli.getChildren(path, true);
    child.sort();
    for (it = child.begin(); it != child.end(); it++)
    {
      cout << "my child is " << (*it) << endl;
      cur = *it; 
      // ret = cli.delete_node(cur.c_str(), -1);
    }
  }
  catch (exception &e)
  {
    cout << "zkException[] " << e.what() << endl;
  }
  catch (...)
  {
    cout << "....." << endl;
  }

  // disconnect
  cli.close();
  std::cout << "disconnected" << std::endl;
  return 0;
}

