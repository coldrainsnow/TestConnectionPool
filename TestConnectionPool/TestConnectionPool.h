#include<string>
#include<queue>
#include<mutex>
using namespace std;
#include "Connection.h"


/*
实现连接池功能模块
*/
class ConnectionPool
{
public:
	// 获取连接池对象实例
	static ConnectionPool* getConnectionPool();
private:
	ConnectionPool(); // 单例#1 构造函数私有化
public:
	// 从配置文件中加载配置项
	bool loadConfigFile(); 

	string _ip;	// mysql的ip地址
	unsigned short _port;// mysql的端口号3306
	string _username;// mysql的登录用户名
	string _password;// mysql的登录密码
	int _initSize; // 连接池的初始化连接量
	int _maxSize;// 连接池的最大连接量
	int _maxIdleTime;// 连接池的最大空闲时间
	int _connectionTimeout;//连接池获取连接的超时时间

	queue<Connection*> connectionQue; //存储mysql连接的队列
	mutex _queueMutex; // 维护连接队列的线程安全的互斥锁
};