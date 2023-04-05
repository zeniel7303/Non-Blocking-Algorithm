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

#ifdef __OPTIMISTIC_SYNCHRONIZATION__
		clist.Recyle_FreeList();
#endif //__OPTIMISTIC_SYNCHRONIZATION__

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
*/

/*
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
*/

/*
3. __OPTIMISTIC_SYNCHRONIZATION__ : 낙천적인 동기화
- 개요
1) 세밀한 동기화의 경우 잠금의 획득과 해제가 너무 빈번해 성능이 너무 느리다.
	[1] 리스트가 길어지는 경우 성능이 더더욱 떨어진다.

- 해결방안
1) 이동 시 잠금을 하지 않는다.
2) 잠금을 획득하지 않고 검색한 후, pred와 curr을 잠그고, 잠긴 노드가 정확한지 확인한다.
	[1] 드문 경우지만, 만약 엉뚱한 노드가 잠길 경우 잠금을 해제하고 다시 시작한다.
	[2] 2개의 노드를 잠그므로 deadlock에 주의해야한다. (lock을 걸고 해제하는 순서에 주의)

- 불안요소
1) 잠금을 획득하지 않고 이동할 경우 삭제된 노드로 이동하는 경우가 생기면?
ex)
while(curr->m_key <= _key) {
	pred = curr; curr = curr->m_next;
}
-> (assembly)
mov eax, curr->m_next
	< 이 곳에서 다른 쓰레드가 curr->next를 제거 >
mov curr, eax

* 이동할 때 락을 걸지 않고 하는데 다른 쓰레드가 삭제하면? -> 프로그램 오작동
next를 읽어 다시 next를 하고 그 사이에 다른 쓰레드가 와서 curr의 next를 뺴면? 무슨 일이 벌어질지 알 수가 없다.(오작동)
delete된 값은 프로그램에서 값이 재사용되므로 어떻게 이동할지 모른다.

- 해결책
1) 제거된 노드를 통해 이동하는 것을 허용하자.
	[1] 제거 = remove, 삭제 = delete
2) remove시 Node를 delete하지 않는다.
	[1] delete하면 next가 어떤 값으로 바뀔지 알 수 없다.
	[2] 정확성은 보장할 수 없으나 안정성은 보장할 수 있다.(적어도 프로그램이 죽진 않는다.)
3) 이동 종료 후, pred와 next가 제대로 자리를 잡았는지 검사해야 한다.
	[1] remove된 node를 거친 이동 이동은 잘못된 결과를 야기할 수 있다.
	[2] 잘못된 결과가 나오면? 처음부터 다시 해야한다.
* 그럼 이러한 것들을 어떻게 보장하는가? -> 유효성 검사를 하자

- Validate() : 유효성 검사
1) 다시 처음부터 이동해서 원래 pred, curr로 다시 올 수 있는지 확인한다.
2) pred->next == curr임을 확인한다.
*위의 검사로 충분안가?
	-> pred와 curr이 locking이 되어 있으므로 충분하다.

- 문제점
1) 낙천적인 동기화 알고리즘은 기아를 겪을 수 있다.
	[1] 쓰레드는 새로운 노드가 반복해서 추가되거나 제거되면 영원히 지연될 수 있다.
2) 기아 상태를 겪는 경우는 흔치 않은 경우이므로 실제로는 잘 동작할 가능성이 크다.
3) lock의 횟수가 비약적으로 감소했으나, 리스트를 두 번 순회해야 한다는 눈에 보이는 오버헤드가 있다.
*/

/*
4. __LAZY_SYNCHRONIZATION__

5. __NONBLOCKING_SYNCHRONIZATION__
*/