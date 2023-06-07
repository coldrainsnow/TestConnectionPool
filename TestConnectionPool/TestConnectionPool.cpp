#include "TestConnectionPool.h"
#include <iostream>
#include "public.h"
#include <thread>
using namespace std;

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	// 对于静态变量的初始化，编译器自动lock和unlock
	static ConnectionPool pool;
	return &pool;
}

bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr) 
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}

	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find("=", 0);
		if (idx == -1)
		{
			continue;
		}
		
		// password=123456\n
		int endidx = str.find('\n', idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeOut")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}

// 连接池的构造
ConnectionPool::ConnectionPool()
{
	// 加载配置项
	if (!loadConfigFile())
	{
		return;
	}

	// 创建初始数量的连接
	for (int i = 0; i < _initSize; ++i)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		_connectionQue.push(p);
		_connectionCnt++;
	}

	// 启动一个新的进程，作为连接的生产者 linux thread底层其实调用的是pthread_create
	// this是绑定当前的对象，给这个成员方法绑定当前的对象，要不然这个成员方法是没办法直接作为线程函数的，因为线程函数都是C接口
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
}

// 运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock); // 队列不空，此处生产线程进入等待状态
		}

		// 连接数量没有到达上限，继续创建新的连接
		if (_connectionCnt < _maxIdleTime)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			_connectionQue.push(p);
			_connectionCnt++;
		}

		// 通知消费者线程，可以消费连接了
		cv.notify_all();
	}
}

// 给外部提供接口，从连接池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		// sleep 这里可以看下wait_for里面的sv_status
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
		{
			// 如果醒来还是空的
			if (_connectionQue.empty())
			{
				LOG("获取空闲连接超时了...获取连接失败!");
				return nullptr;
			}
		}
	}

	/*
	shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
	调用connection的析构函数，connection就被close掉了
	这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
	*/

	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
			// 这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
			unique_lock<mutex> lock(_queueMutex);
			_connectionQue.push(pcon);
		});
	_connectionQue.pop();
	cv.notify_all();	// 消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接，说明现在并发量比较大
	return sp;


}