#pragma once
#include "stdafx.h"

#ifdef __COARSE_GRAINED_SYNCHRONIZATION__
class CNode
{
public:
	int m_key;
	CNode* m_next;

	CNode() { m_next = NULL; }
	CNode(int _keyValue)
	{
		m_next = NULL;
		m_key = _keyValue;
	}

	~CNode() {}
};

class CLinkedList
{
private:
	CNode m_head, m_tail;
	std::mutex m_lock;

public:
	CLinkedList();
	~CLinkedList();

	void Init();
	bool Add(int _key);
	bool Remove(int _key);
	bool Contains(int _key);

	void display20()
	{
		int c = 20;
		CNode* p = m_head.m_next;
		while (p != &m_tail)
		{
			cout << p->m_key << ", ";
			p = p->m_next;
			c--;
			if (c == 0)
				break;
		}
		cout << endl;
	}
};
#endif //__COARSE_GRAINED_SYNCHRONIZATION__

#ifdef __FINE_GRAINED_SYNCHRONIZATION__
class CNode
{
public:
	int m_key;
	CNode* m_next;
	std::mutex m_lock;

	CNode() { m_next = NULL; }
	CNode(int _keyValue)
	{
		m_next = NULL;
		m_key = _keyValue;
	}

	~CNode() {}

	void Lock() { m_lock.lock(); }
	void Unlock() { m_lock.unlock(); }
};

class CLinkedList
{
private:
	CNode m_head, m_tail;

public:
	CLinkedList();
	~CLinkedList();

	void Init();
	bool Add(int _key);
	bool Remove(int _key);
	bool Contains(int _key);

	void display20()
	{
		int c = 20;
		CNode* p = m_head.m_next;
		while (p != &m_tail)
		{
			cout << p->m_key << ", ";
			p = p->m_next;
			c--;
			if (c == 0)
				break;
		}
		cout << endl;
	}
};
#endif //__FINE_GRAINED_SYNCHRONIZATION__

#ifdef __OPTIMISTIC_SYNCHRONIZATION__
class CNode
{
public:
	int m_key;
	CNode* m_next;
	std::mutex m_lock;

	CNode() { m_next = NULL; }
	CNode(int _keyValue)
	{
		m_next = NULL;
		m_key = _keyValue;
	}

	~CNode() {}

	void Lock() { m_lock.lock(); }
	void Unlock() { m_lock.unlock(); }
};

class CLinkedList
{
private:
	CNode m_head, m_tail;
	CNode* m_freeList;
	CNode m_freeTail;
	mutex m_lock;

public:
	CLinkedList();
	~CLinkedList();

	void Init();
	bool Add(int _key);
	bool Remove(int _key);
	bool Contains(int _key);
	bool Validate(CNode* _pred, CNode* _curr);
	void Recyle_FreeList();

	void display20()
	{
		int c = 20;
		CNode* p = m_head.m_next;
		while (p != &m_tail)
		{
			cout << p->m_key << ", ";
			p = p->m_next;
			c--;
			if (c == 0)
				break;
		}
		cout << endl;
	}
};
#endif //__OPTIMISTIC_SYNCHRONIZATION__

#ifdef __LAZY_SYNCHRONIZATION__
#endif //__LAZY_SYNCHRONIZATION__

#ifdef __NONBLOCKING_SYNCHRONIZATION__
#endif //__NONBLOCKING_SYNCHRONIZATION__