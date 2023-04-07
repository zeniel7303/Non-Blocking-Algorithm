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
	CNode* head, * tail;
	mutex glock;
public:
	CQueue()
	{
		head = tail = new CNode(0);
	}
	~CQueue() {}

	void Init()
	{
		CNode* ptr;
		while (head->next != nullptr) {
			ptr = head->next;
			head->next = head->next->next;
			delete ptr;
		}
		tail = head;
	}

	void Enq(int key)
	{
		CNode* e = new CNode(key);
		glock.lock();
		tail->next = e;
		tail = e;
		glock.unlock();
	}

	int Deq()
	{
		glock.lock();
		if (nullptr == head->next) {
			cout << "QUEUE EMPTY!!\n";
			while (true);
		}
		int result = head->next->key;
		CNode* temp = head;
		head = head->next;
		glock.unlock();
		delete temp;
		return result;
	}

	void display20()
	{
		int c = 20;
		CNode* p = head->next;
		while (p != nullptr)
		{
			cout << p->key << ", ";
			p = p->next;
			c--;
			if (c == 0) break;
		}
		cout << endl;
	}
};
#endif
#endif
