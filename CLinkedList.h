#pragma once
#include "stdafx.h"

#ifdef __LINKEDLIST__
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
class CNode
{
public:
	int m_key;
#ifdef __Shared_Ptr__
	shared_ptr<CNode> m_next;
#else
	CNode* m_next;
#endif //__Shared_Ptr__	
	std::mutex m_lock;
	bool m_removed;

	CNode() { m_next = NULL; m_removed = false; }
	CNode(int _keyValue)
	{
		m_next = NULL;
		m_key = _keyValue;
		m_removed = false;
	}

	~CNode() {}

	void Lock() { m_lock.lock(); }
	void Unlock() { m_lock.unlock(); }
};

class CLinkedList
{
private:
#ifdef __Shared_Ptr__
	shared_ptr<CNode> m_head, m_tail;
#else
	CNode m_head, m_tail;
	CNode* m_freeList;
	CNode m_freeTail;
	mutex m_lock;
#endif //__Shared_Ptr__

public:
	CLinkedList();
	~CLinkedList();

	void Init();
	bool Add(int _key);
	bool Remove(int _key);
	bool Contains(int _key);
#ifdef __Shared_Ptr__
	bool Validate(shared_ptr<CNode> _pred, shared_ptr<CNode> _curr);
#else
	bool Validate(CNode* _pred, CNode* _curr);
	void Recyle_FreeList();
#endif //__Shared_Ptr__

	void display20()
	{
		int c = 20;
#ifdef __Shared_Ptr__
		shared_ptr<CNode> p = m_head->m_next;
		while (p != m_tail)
#else
		CNode* p = m_head.m_next;
		while (p != &m_tail)
#endif //__Shared_Ptr__
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
#endif //__LAZY_SYNCHRONIZATION__

#ifdef __NONBLOCKING_SYNCHRONIZATION__
class LFNode;

class MPTR
{
public:
	INT_PTR m_value;

public:
	void Set(LFNode* _node, bool _removed)
	{
		m_value = reinterpret_cast<INT_PTR>(_node);
		if (m_value == true)
		{
			m_value = m_value | 0x01;
		}
		else
		{
#ifdef _WIN64
			m_value = m_value & 0xFFFFFFFFFFFFFFFE;
#else
			m_value = m_value & 0xFFFFFFFE;
#endif
		}
	}

	LFNode* GetPtr()
	{
#ifdef _WIN64
		return reinterpret_cast<LFNode*>(m_value & 0xFFFFFFFFFFFFFFFE);
#else
		return reinterpret_cast<LFNode*>(m_value & 0xFFFFFFFE);
#endif	
	}

	LFNode* GetPtr(bool* _removed)
	{
		INT_PTR temp = m_value;
		if ((temp & 0x1) == 0)
		{
			*_removed = false;
		}
		else
		{
			*_removed = true;
		}

#ifdef _WIN64
		return reinterpret_cast<LFNode*>(temp & 0xFFFFFFFFFFFFFFFE);
#else
		return reinterpret_cast<LFNode*>(temp & 0xFFFFFFFE);
#endif	
	}

	bool CAS(LFNode* _oldNode, LFNode* _newNode, bool _oldRemoved, bool _newRemoved)
	{
		INT_PTR oldValue, newValue;

		oldValue = reinterpret_cast<INT_PTR>(_oldNode);
		if (_oldRemoved == true)
		{
			oldValue = oldValue | 0x01;
		}
		else
		{
#ifdef _WIN64
			oldValue = oldValue & 0xFFFFFFFFFFFFFFFE;
#else
			oldValue = oldValue & 0xFFFFFFFE;
#endif
		}

		newValue = reinterpret_cast<INT_PTR>(_newNode);
		if (_newRemoved == true)
		{
			newValue = newValue | 0x01;
		}
		else
		{
#ifdef _WIN64
			newValue = newValue & 0xFFFFFFFFFFFFFFFE;
#else
			newValue = newValue & 0xFFFFFFFE;
#endif
		}

#ifdef _WIN64
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int64_t*>(&m_value), &oldValue, newValue);
#else
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&m_value), &oldValue, newValue);
#endif

	}

	bool TryMarking(LFNode* _oldNode, bool _newRemoved)
	{
		INT_PTR oldValue, newValue;
		oldValue = reinterpret_cast<INT_PTR>(_oldNode);
#ifdef _WIN64
		oldValue = oldValue & 0xFFFFFFFFFFFFFFFE;
#else
		oldValue = oldValue & 0xFFFFFFFE;
#endif

		newValue = oldValue;
		if (_newRemoved == true)
		{
			newValue = newValue | 0x01;
		}

#ifdef _WIN64
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int64_t*>(&m_value), &oldValue, newValue);
#else
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_int*>(&m_value), &oldValue, newValue);
#endif
	}
};

class LFNode
{
public:
	int m_key;
	MPTR m_next;

public:
	LFNode() { m_next.Set(nullptr, false); }
	LFNode(int _keyValue)
	{
		m_next.Set(nullptr, false);
		m_key = _keyValue;
	}
	LFNode& operator=(const LFNode& rhs)
	{
		if (this != &rhs) return *this;
	}
};

class LFList
{
public:
	LFNode m_head, m_tail;
	LFNode* m_freeList;
	LFNode m_freeTail;
	mutex m_lock;

public:
	LFList();
	~LFList();

	void Init();
	void RecycleFreeList();
	void Find(int _key, LFNode* (&_pred), LFNode* (&_curr));
	bool Add(int _key);
	bool Remove(int _key);
	bool Contains(int _key);
	void Display(int _num);
};
#endif //__NONBLOCKING_SYNCHRONIZATION__
#endif