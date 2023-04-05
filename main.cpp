#include "stdafx.h"
#include "CLinkedList.h"

const auto NUM_TEST = 4000000;
const auto KEY_RANGE = 1000;

CLinkedList clist;
volatile int sum;

void ThreadFunc(int _numThread)
{
	int key;

	for (int i = 0; i < NUM_TEST / _numThread; i++)
	{
		switch (rand() % 3)
		{
		case 0:
			key = rand() % KEY_RANGE;
			clist.Add(key);
			break;
		case 1:
			key = rand() % KEY_RANGE;
			clist.Remove(key);
			break;
		case 2:
			key = rand() % KEY_RANGE;
			clist.Contains(key);
			break;
		default:
			cout << "Error \n";
			exit(-1);
			break;
		}
	}
}

void main()
{
#ifdef __COARSE_GRAINED_SYNCHRONIZATION__
#endif //__COARSE_GRAINED_SYNCHRONIZATION__

#ifdef __FINE_GRAINED_SYNCHRONIZATION__
#endif //__FINE_GRAINED_SYNCHRONIZATION__

#ifdef __OPTIMISTIC_SYNCHRONIZATION__
#endif //__OPTIMISTIC_SYNCHRONIZATION__

#ifdef __LAZY_SYNCHRONIZATION__
#endif //__LAZY_SYNCHRONIZATION__

#ifdef __NONBLOCKING_SYNCHRONIZATION__
#endif //__NONBLOCKING_SYNCHRONIZATION__

	for (int i = 1; i <= 16; i *= 2)
	{
		clist.Init();
		sum = 0;
		vector<thread> threads;

		auto startTime = high_resolution_clock::now();

		for (int j = 0; j < i; j++)
		{
			threads.emplace_back(ThreadFunc, i);
		}

		for (auto& th : threads)
		{
			th.join();
		}

		auto endTime = high_resolution_clock::now();
		threads.clear();
		auto execTime = endTime - startTime;

		int execMs = duration_cast<milliseconds>(execTime).count();

		clist.display20();
		cout << "Threads[ " << i << " ] , sum = " << sum;
		cout << ", execTime =" << execMs << " msecs\n";
	}

	system("pause");
}

/*
1. __COARSE_GRAINED_SYNCHRONIZATION__ : 성긴 동기화
- 구현
1) 리스트는 하나의 잠금을 갖고 있으며, 모든 메서드 호출은 이 잠금을 통해 Ciritical Section으로 진행된다.
	[1] 모든 메서드는 잠금을 가지고 있는 동안에만 리스트ㄴㄴㄴ에 접근한다.
- 문제점
1) 경쟁이 낮을 경우 큰 오버헤드가 없어 이 동기화가 좋은 선택이지만 경쟁이 높아질 경우 성능이 저하된다.(향상이 없다.)
2) Blocking이다.

굉장히 심플하고 알아보기 쉬우나 성능이 안나옴.(멀티쓰레드에서 성능 향상이 없다.)

2. __FINE_GRAINED_SYNCHRONIZATION__ : 세밀한 동기화
- 구현
1) 전체 리스트를 한꺼번에 잠그는 것보다 노드를 잠그는 것이 병행성을 향상시킬 수 있다.
	[1] 전체 리스트에 대한 잠금을 두는 것이 아니라 각각의 노드에 잠금을 둔다.
	[2] Node에 Lock()과 Unlock() 메소드를 구현해야 한다.
	[3] Node의 next field를 변경할 경우에는 반드시 Lock()을 얻은 후 변경해야 한다.

- 주의점
1) add와 remove 시점의 pred, curr가 가리키는 노드는 locking이 되어 있어야 한다.
2) head부터 node 이동을 할 때 lock을 잠그면서 이동해야 한다.
	[1] 예를 들어 a의 잠금을 풀고 나서 b(a->next였던)의 잠금을 한다면 그 사이에 다른 스레드에 의해 b가 제거될 수 있기 떄문이다.
	[2] 즉, 이동 시 pred가 잠금 상태일 때 동안 curr의 잠금을 획득한다.

- 문제점
1) 성능이 너무 느리다.(락을 자주 걸기 때문에)
	-> 그럼 락을 자주 안거는 방법은 무엇인가? -> 낙천적인 동기화

3. __OPTIMISTIC_SYNCHRONIZATION__ : 낙천적인 동기화

4. __LAZY_SYNCHRONIZATION__

5. __NONBLOCKING_SYNCHRONIZATION__
*/