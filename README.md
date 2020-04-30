ZooKeeper is a distributed co-ordination service to manage large set of hosts. Co-ordinating and managing a service in a distributed environment is a complicated process. ZooKeeper solves this issue with its simple architecture and API. ZooKeeper allows developers to focus on core application logic without worrying about the distributed nature of the application.  

There are two modes in which Zookeeper runs: standalone (single server) and quorum (ensemble).
We should always shoot for an odd number of servers in ensemble.
Namespace : znode ==>(sequential) persistent or (sequential) ephemeral (ephemeral ZNodes are not allowed to have children).  Sequential: The format of the counter is %010d — basically, it is 10 digits with 0 (zero) padding.
ZooKeeper watches are a simple mechanism for the client. Also, we can say a watch is a one-shot operation, means it triggers one notification. However, a client can set a new watch upon receiving each notification just to receive multiple notifications over time.

1. ZooKeeper ensemble starts and wait for the client to connect
2. Client tries to connect (leader or follower node)
       If connected then ensemble assign client a session ID and send “ack” back
       else client does NOT receive “ack” then
       	try to connect another node in ensemble
	*** Write:Basically, leader node handles write process. In addition, to all the Znodes, leader forwards the 
       write request and then waits for answers from the znodes. You can be sure about the writing
       process is complete if half of the Znodes reply.
              Read: By a specifically connected Znode, reads are performed internally, hence we do not need
        interaction with the cluster.
	       Replicated Database: In zookeeper, to store data we use Replicated Database. However, every Znode
       has the same data at every time with the help of consistency and each Znode has its own
       database.
              Leader: For processing write requests, the leader is the Znode which is responsible.
              Follower: Generally, it receives write requests from the clients and then forwards them to the leader
       Znode.
	       Request Processor: In the leader node, it is present. And, from the follower node, it governs write
       Requests.
              Atomic Broadcasts: The reasons behind changes from the leader node to the follower nodes are 
       Atomic broadcasts.
3. Once connect to a node, the client will send heartbeat to the node in a particular interval to make sure that the connection is not lost.


Currently there are five objects to wrap C-binding API and described as followings:
1. zkwatcher.cpp:
        start()
        run()
2. zkconnect.cpp:
        connect to zookeeper ensemble, create znode (persistent [sequential], ephemeral [sequential]),  
        delete znode, watch znode and getChildren.
3. zkclinet.cpp:
        client connect to zookeeper ensemble, create znode, delete znode, watch znode. getchildren, and
        re-connect ...etc.
4. zkdlock.cpp:
        The process to obtain a lock. After the lock was acquired by the client, it can access the shared 
        resources.
        The trip is completed.  The lock is obtained by the minimum sub new znode.
5. zkexception.cpp:

Test:
1. client/server connection: unexpected server(s) down, then its clients become disconnected client which 
  will automatically tries to re-connect to other alive (either leader or follower) servers in ZooKeeper 
  ensemble.
2. client/server connection: unexpected client(s) down, then other client(s) awake to obtain shared
  resources and resume unfinished task.
3. Finally, real project, four OqS connected to ZooKeeper ensemble and only one obtains resource and
  process the task.
   a. client connects to ZooKeeper ensemble.
   b. receive session ID and "ack" or re-connect if not received.
   c. send heartbeat to keep connection.
   d. check whether lock (eq. "/lock") znode exists or not, create one persistent znode if not exists.
   e. creates an ephemeral sequential znode under lock znode.
   f. try to obtain distributed lock based on the minimum sub of children znodes.
   g. win to get lock then access the shared resources and process the task.
   h. not won, wait, creates a watcher to monitor preceding znode.
   i. when winner failure caused by unexpected situation, ZooKeeper ensemble will send the notification to 
    the client for which znode is watching, client awake to obtain resources to resume the task.


API to obtain distributed lock:
// get "/lock" to m_dLockPath, assign "dlock" to m_dLockName
// connect to zookeeper and create persistent "/test1" node
zkdlock dlock(hostname, lockpath, lockname, watchPrecedingNode);

// disconnect
dlock.close();

API to monitor which pid holds a key
// find a pid which holds a key
bool monitor = dlock.dlockMonitor();
