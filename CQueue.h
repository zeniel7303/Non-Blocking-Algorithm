#pragma once
#include "stdafx.h"

#ifdef __QUEUE__
#ifdef __COARSE_GRAINED_SYNCHRONIZATION__
class CNode 
{
public:
	int key;
	CNode* next;

	CNode() { next = NULL; }
	CNode(int key_value) {
		next = nullptr;
		key = key_value;
	}
	~CNode() {}
};

class CNullmutex {
public:
	void lock() {}
	void unlock() {}
};

class CQueue {
	// 멤버로 헤드와 테일을 갖는다.
	// enq할 땐 tail에 붙이고, deq 할 땐 head에서 뽑는다.
	CNode* head, * tail;
	mutex glock;
public:
	CQueue();
	~CQueue();

	void Init();
	void Enq(int key);
	int Deq();
	void Display20();
};
#endif
#ifdef __LOCK_FREE__
class CNode
{
public:
	int m_key;
	CNode* m_next;

	CNode() { m_next = nullptr; }
	CNode(int _keyValue)
	{
		m_next = nullptr;
		m_key = _keyValue;
	}
	~CNode() {}
};

class CLockFreeQueue
{
private:
	CNode* volatile m_head;
	CNode* volatile m_tail;

public:
	CLockFreeQueue();
	~CLockFreeQueue();

	void Init();
	bool CAS(CNode* volatile* _addr, CNode* _oldNode, CNode* _newNode);
	void Enq(int _key);
	int Deq();
	int EmptyError();
	void Display20();
};
#endif
#endif
