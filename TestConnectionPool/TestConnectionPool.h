#include<string>
#include<queue>
#include<mutex>
using namespace std;
#include "Connection.h"


/*
ʵ�����ӳع���ģ��
*/
class ConnectionPool
{
public:
	// ��ȡ���ӳض���ʵ��
	static ConnectionPool* getConnectionPool();
private:
	ConnectionPool(); // ����#1 ���캯��˽�л�
public:
	// �������ļ��м���������
	bool loadConfigFile(); 

	string _ip;	// mysql��ip��ַ
	unsigned short _port;// mysql�Ķ˿ں�3306
	string _username;// mysql�ĵ�¼�û���
	string _password;// mysql�ĵ�¼����
	int _initSize; // ���ӳصĳ�ʼ��������
	int _maxSize;// ���ӳص����������
	int _maxIdleTime;// ���ӳص�������ʱ��
	int _connectionTimeout;//���ӳػ�ȡ���ӵĳ�ʱʱ��

	queue<Connection*> connectionQue; //�洢mysql���ӵĶ���
	mutex _queueMutex; // ά�����Ӷ��е��̰߳�ȫ�Ļ�����
};