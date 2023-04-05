#include "CLinkedList.h"

#ifdef __COARSE_GRAINED_SYNCHRONIZATION__
CLinkedList::CLinkedList()
{
	// Head는 -무한대(int 중 가장 작은 값)
	m_head.m_key = 0x80000000;
	// Tail은 +무한대(int 중 가장 큰 값)
	m_tail.m_key = 0x7fffffff;
	m_head.m_next = &m_tail;
}

CLinkedList::~CLinkedList()
{

}

void CLinkedList::Init()
{
	CNode* ptr;
	while (m_head.m_next != &m_tail)
	{
		ptr = m_head.m_next;
		m_head.m_next = m_head.m_next->m_next;
		delete ptr;
	}
}

bool CLinkedList::Add(int _key)
{
	// predication(비교할 때 보조하는 연산) (함수에서도 적용)
	CNode* pred;
	CNode* curr;

	// 안에 있어도 되지만 굳이 넣을 이유는 없다.(락과 언락 사이의 코드는 작을 수록 좋다.)
	// 헤드의 값을 다른 스레드가 바꿀 순 있으나 헤드의 주소는 절대 바뀌지 않는다.
	// 이 객체가 사라지지 않느 ㄴ한 절대로 바뀌지 않는 값을 pred에 넣으므로 lock 안에 넣을 이유가 없다.
	pred = &m_head;
	m_lock.lock();

	curr = pred->m_next;

	// _key보다 작으면 계속 전진
	while (curr->m_key < _key)
	{
		pred = curr;
		curr = curr->m_next;
	}

	// 이미 존재 함.
	if (_key == curr->m_key)
	{
		m_lock.unlock();

		return false;
	}
	// 새로운 노드를 만들어 추가
	else
	{
		CNode* node = new CNode(_key);
		node->m_next = curr;
		pred->m_next = node;
		m_lock.unlock();

		return true;
	}
}

bool CLinkedList::Remove(int _key)
{
	CNode* pred;
	CNode* curr;

	pred = &m_head;
	m_lock.lock();

	curr = pred->m_next;
	while (curr->m_key < _key)
	{
		pred = curr;
		curr = curr->m_next;
	}

	if (_key == curr->m_key)
	{
		pred->m_next = curr->m_next;
		delete curr;
		m_lock.unlock();
		return true;
	}
	else
	{
		m_lock.unlock();
		return false;
	}
}

bool CLinkedList::Contains(int _key)
{
	CNode* pred;
	CNode* curr;

	pred = &m_head;
	m_lock.lock();

	curr = pred->m_next;
	while (curr->m_key < _key)
	{
		pred = curr;
		curr = curr->m_next;
	}

	if (_key == curr->m_key)
	{
		m_lock.unlock();
		return true;
	}
	else
	{
		m_lock.unlock();
		return false;
	}
}
#endif //__COARSE_GRAINED_SYNCHRONIZATION__

#ifdef __FINE_GRAINED_SYNCHRONIZATION__
CLinkedList::CLinkedList()
{
	m_head.m_key = 0x80000000;
	m_tail.m_key = 0x7fffffff;
	m_head.m_next = &m_tail;
}

CLinkedList::~CLinkedList()
{

}

void CLinkedList::Init()
{
	CNode* ptr;
	while (m_head.m_next != &m_tail)
	{
		ptr = m_head.m_next;
		m_head.m_next = m_head.m_next->m_next;
		delete ptr;
	}
}

bool CLinkedList::Add(int _key)
{
	// predication(비교할 때 보조하는 연산) (함수에서도 적용)
	CNode* pred;
	CNode* curr;

	// head부터 이동할 때 lock을 걸고 head를 얻음.
	m_head.Lock();
	pred = &m_head;
	// curr은 pred next하고 락을 걸어둔다.
	curr = pred->m_next;
	curr->Lock();

	while (curr->m_key < _key)
	{
		// while문을 돌떄 pred를 언락하고 curr을 집어넣는다.
		// 어차피 이동해야하므로 필요 없어서 언락하는 것이다.
		pred->Unlock();
		pred = curr;
		//curr을 다시 잠그고 next를 다른 애가 바꾸지 못하게 만든다.
		curr = curr->m_next;
		curr->Lock();
	}

	if (_key == curr->m_key)
	{
		curr->Unlock();
		pred->Unlock();

		return false;
	}
	else
	{
		CNode* node = new CNode(_key);
		node->m_next = curr;
		pred->m_next = node;
		curr->Unlock();
		pred->Unlock();

		return true;
	}
}

bool CLinkedList::Remove(int _key)
{
	CNode* pred;
	CNode* curr;

	m_head.Lock();
	pred = &m_head;
	curr = pred->m_next;
	curr->Lock();

	while (curr->m_key < _key)
	{
		pred->Unlock();
		pred = curr;
		curr = curr->m_next;
		curr->Lock();
	}

	if (_key == curr->m_key)
	{
		pred->m_next = curr->m_next;
		curr->Unlock();
		delete curr;
		pred->Unlock();
		return true;
	}
	else
	{
		curr->Unlock();
		pred->Unlock();
		return false;
	}
}

bool CLinkedList::Contains(int _key)
{
	CNode* pred;
	CNode* curr;

	m_head.Lock();
	pred = &m_head;
	curr = pred->m_next;
	curr->Lock();

	while (curr->m_key < _key)
	{
		pred->Unlock();
		pred = curr;
		curr = curr->m_next;
		curr->Lock();
	}

	if (_key == curr->m_key)
	{
		curr->Unlock();
		pred->Unlock();
		return true;
	}
	else
	{
		curr->Unlock();
		pred->Unlock();
		return false;
	}
}
#endif //__FINE_GRAINED_SYNCHRONIZATION__

#ifdef __OPTIMISTIC_SYNCHRONIZATION__
CLinkedList::CLinkedList()
{
	m_head.m_key = 0x80000000;
	m_tail.m_key = 0x7FFFFFFF;
	m_head.m_next = &m_tail;

	m_freeTail.m_key = 0x7FFFFFFF;
	m_freeList = &m_freeTail;
}

CLinkedList::~CLinkedList()
{

}

void CLinkedList::Init()
{
	CNode* ptr;
	while (m_head.m_next != &m_tail)
	{
		ptr = m_head.m_next;
		m_head.m_next = m_head.m_next->m_next;
		delete ptr;
	}
}

bool CLinkedList::Add(int _key)
{
	// predication(비교할 때 보조하는 연산) (함수에서도 적용)
	CNode* pred;
	CNode* curr;

	pred = &m_head;
	curr = pred->m_next;

	while (curr->m_key < _key)
	{
		pred = curr;
		curr = curr->m_next;
	}

	// 유효성 검사 전에 pred와 curr을 lock
	pred->Lock();
	curr->Lock();

	if (Validate(pred, curr))
	{
		if (_key == curr->m_key)
		{
			curr->Unlock();
			pred->Unlock();

			return false;
		}
		else
		{
			CNode* node = new CNode(_key);
			node->m_next = curr;
			pred->m_next = node;
			curr->Unlock();
			pred->Unlock();

			return true;
		}
	}

	curr->Unlock();
	pred->Unlock();

	return false;
}

bool CLinkedList::Remove(int _key)
{
	CNode* pred;
	CNode* curr;

	pred = &m_head;
	curr = pred->m_next;

	while (curr->m_key < _key)
	{
		pred = curr;
		curr = curr->m_next;
	}

	pred->Lock();
	curr->Lock();

	if (Validate(pred, curr))
	{
		if (_key == curr->m_key)
		{
			pred->m_next = curr->m_next;

			// 추가
			m_lock.lock();
			curr->m_next = m_freeList;
			m_freeList = curr;
			m_lock.unlock();

			pred->Unlock();
			curr->Unlock();
			//delete curr;

			return true;
		}
		else
		{
			pred->Unlock();
			curr->Unlock();

			return false;
		}
	}

	curr->Unlock();
	pred->Unlock();

	return false;
}

bool CLinkedList::Contains(int _key)
{
	CNode* pred;
	CNode* curr;

	pred = &m_head;
	curr = pred->m_next;

	while (curr->m_key < _key)
	{
		pred = curr;
		curr = curr->m_next;
	}

	pred->Lock();
	curr->Lock();

	if (Validate(pred, curr))
	{
		curr->Unlock();
		pred->Unlock();

		return _key == curr->m_key;
	}

	curr->Unlock();
	pred->Unlock();

	return false;
}

bool CLinkedList::Validate(CNode* _pred, CNode* _curr)
{
	// 유효성 검사를 어떻게 하는 것이 중요하다.
	// 먼저 헤드부터 쭉 따라간다.
	CNode* node = &m_head;

	while (node->m_key <= _pred->m_key)
	{
		// 찾았다.
		if (node == _pred)
		{
			// pred 다음 노드가 curr인지 체크
			return _pred->m_next == _curr;
		}

		node = node->m_next;
	}

	// 쭉 이동을 하다보니 pred를 만나지 못했는데 key가 pred보다 크다면 실패.
	return false;
}

/*
- 낙천적인 동기화의 문제
1. 언제 delete를 하는가?

-해결책
1. freeList를 만들어 거기에 넣어 놓는다. (쭉 모아두고 나중에 delete할 용도)
2. next field의 접근 시 exception을 발생시키지 않도록만 하면 된다.
	[1] freeList의 마지막 node는 MAXINT를 갖는다.
*/
void CLinkedList::Recyle_FreeList()
{
	CNode* a = m_freeList;
	while (a != &m_freeTail)
	{
		CNode* b = a->m_next;
		delete a;
		a = b;
	}

	m_freeList = &m_freeTail;
}
#endif //__OPTIMISTIC_SYNCHRONIZATION__

#ifdef __LAZY_SYNCHRONIZATION__
#endif //__LAZY_SYNCHRONIZATION__

#ifdef __NONBLOCKING_SYNCHRONIZATION__
#endif //__NONBLOCKING_SYNCHRONIZATION__