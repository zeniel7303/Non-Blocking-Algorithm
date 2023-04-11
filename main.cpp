#include "stdafx.h"
#include "CLinkedList.h"
#include "CQueue.h"

const auto NUM_TEST = 10000000;
const auto KEY_RANGE = 1000;

#ifdef __LINKEDLIST__
#ifdef __NONBLOCKING_SYNCHRONIZATION__
LFList cTemp;
#else
CLinkedList cTemp;
#endif
#else
#ifdef __LOCK_FREE__
CLockFreeQueue cTemp;
#else
CQueue cTemp;
#endif
#endif

volatile int sum;

void ThreadFunc(int _numThread)
{
#ifdef __LINKEDLIST__
	int key;

	for (int i = 0; i < NUM_TEST / _numThread; i++)
	{
		switch (rand() % 3)
		{
		case 0:
			key = rand() % KEY_RANGE;
			cTemp.Add(key);
			break;
		case 1:
			key = rand() % KEY_RANGE;
			cTemp.Remove(key);
			break;
		case 2:
			key = rand() % KEY_RANGE;
			cTemp.Contains(key);
			break;
		default:
			cout << "Error \n";
			exit(-1);
			break;
		}
	}
#else
	for (int i = 0; i < NUM_TEST / _numThread; i++) {
		if ((rand() % 2 == 0) || i < 10000 / _numThread) {
			cTemp.Enq(i);
		}
		else {
			int key = cTemp.Deq();
		}
	}
#endif
}

void main()
{
	for (int i = 1; i <= 16; i *= 2)
	{
		cTemp.Init();
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

#ifdef __LINKEDLIST__
#ifdef __NONBLOCKING_SYNCHRONIZATION__
		cTemp.Display(10);
#else
		cTemp.display20();
#endif

#ifdef __OPTIMISTIC_SYNCHRONIZATION__ 
		cTemp.Recyle_FreeList();
#endif //__OPTIMISTIC_SYNCHRONIZATION__

#ifdef __LAZY_SYNCHRONIZATION__
#ifdef __Shared_Ptr__
#else
		cTemp.Recyle_FreeList();
#endif //__Shared_Ptr__

#endif //__LAZY_SYNCHRONIZATION__

#ifdef __NONBLOCKING_SYNCHRONIZATION__
		cTemp.RecycleFreeList();
#endif
#else
		cTemp.Display20();
#endif

		cout << i << " Threads, ";
		cout << "Duration =" << execMs << " msecs. \n";
	}

	system("pause");
}
// __LINKEDLIST__

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
4. __LAZY_SYNCHRONIZATION__ : 게으른 동기화
- 개요
1) 낙천적인 동기화는 lock의 횟수는 비약적으로 감소했으나, 리스트를 두 번 순회해야 한다는 눈에 보이는 오버헤드가 있다.
2) 이를 극복하여 다시 순회하지 않는 알고리즘을 작성하였다.
	[1] Validate()가 노드를 처음부터 다시 순회하지 않고 Validation을 수행한다.
	[2] pred와 curr의 잠금은 여전히 필요하다.
	[3] Contains을 하다보니 Validate 검색을 하는데 lock을 걸지 않아도 될 것 같아 Wait-Free로 만들어 준다.

- 아이디어
1) 각 노드에서 Marked(Removed) 필드를 추가해 그 노드가 집합에서 제거되어 있는지 표시한다.
	[1] Marked(Removed)가 true면 제거되었다는 표시
	[2] Marking을 실제 제거보다 반드시 먼저 수행한다.
	[3] 순회를 할 때 대상 노드를 잠글 필요가 없고 노드가 head에서 접근할 수 있는지 확인하기 위해 전체 리스트를 다시 순회하지 않아도 된다.

- 단점
1) 여전히 Add 메소드와 Remove 메소드는 Blocking이다.
2) 한 쓰레드가 lock을 얻은 채로 지연되면, 다른 쓰레드 역시 지연된다.

- 의문
1) 이 로직이 제대로 돌아갈까?
	[1] 다음의 명제를 주목
		# Marking되어 있지 않은 모든 Node는 실제 리스트에 존재하는 살아있는 Node이다.
			but, true라고 무조건 존재하진 않는다.
		ex)
		Marked = true;
		pred ->next = curr->next;
		이러면 Removed가 false면 무조건 리스트 상에 존재한다.
		하지만 아직 실행되지 않은 이 중간에서 true인데 아직 remove가 되지 않은 요소가 있을 수 있다.
		근데 false인데 Removed 되는 건 절대 없다.
	[2] 보충
		# Validate에서의 Marking 검사는 locking 이후에 이루어지므로 Validate가 OK면 안전하다.

- 주의점
1) FreeList는 어떻게 구현되는가?
2) 재사용 금지
	[1] 모든 메쏘드 호출이 종료된 후 재사용
3) Marking이 해제되는 순간 오작동
4) c++11의 shared_ptr
	[1] 멀티쓰레드에서의 안정성을 책임질 수 없다.
	[2] 성능상의 큰 문제가 있다.

-Shared_ptr 사용
1) FreeList를 통해 재사용 하는건 게임 실행 중엔 사용 불가능하다. 그래서 사용해봄
2) 알아서 delete 해주는 shared_ptr은 멀티쓰레드에서 사용 가능할까?
3) 싱글쓰레드에서 너무나도 느리다.(오버헤드가 너무 크기때문)
4) 멀티쓰레드에 경우 결과가 안나온다. 무한루프에 빠진 듯 하다.
   그래서 갯수를 줄이고 실행하면 결과는 나오나 엑세스 위반이 뜬다.(데이터 레이스가 나서 죽어버림)
   데이터 레이스가 난 곳은 Add의 curr = pred->m_next부분이다.
   head의 next는 shared_ptr이다. 이 shared_ptr을 curr에 복사하는 곳에서 데이터레이스가 발생한다.
   head의 next는 다른 쓰레드가 lock을 걸고 바꾼다. 수정을 계속 하는데 lock을 걸고 읽지를 않는다. shared_ptr로 복사하는 것은 안전하지 않다.
   그래서 읽으려면 락을 걸고 읽어줘야 한다. head의 값은 계속 바뀐다. 여러 쓰레드가 lock을 걸고 바꾸고 있다. 근데 읽는거 락 안걸고 막 읽는 것은 데이터레이스다.
   다른 애가 pred - curr에서 curr 값을 바꾸거나 하지 않아 죽을 일이 없지만 curr = pred->next 여기서는 next에 뭐가 들어있는지 모른다. 그래서 여기서는 죽을 수 있다.
5) 결론은 속도도 안나오고 죽는 문제도 있어서 쓰면 안된다.
*/

/*
5. __NONBLOCKING_SYNCHRONIZATION__ : 비멈춤 동기화
lock-free 알고리즘을 사용하지 않으면 병렬성이 감소하고 우선순위 역전이나 Convoying 현상이 생긴다.

- lock-free 알고리즘이란?
1) 여러개의 쓰레드에서 동시에 호출했을 때에도 정해진 단위 시간마다 적어도 한 개의 호출이 완료되는 알고리즘
2) 자료구조 및 그것에 대한 접근 방법
	 자료구조 | 알고리즘
	[1] queue : enqueue, dequeue
	[2] stack : push, pop
	[3] 이진 트리 : insert, delete, search
3) 멀티쓰레드에서 동시에 호출해도 정확한 결과를 만들어주는 알고리즘
	[1] stl은 타락이다. why? 멀티쓰레드에서는 잘 돌아가지 않는다.(atomic해야하므로)
4) non-blocking 알고리즘
	[1] 다른 쓰레드가 어떤 상태에 있건 상관없이 호출이 완료된다.
5) 호출이 다른 쓰레드와 충돌하였을 경우 적어도 하나의 승자가 있어서, 승자는 delay없이 완료된다.

* 3,4,5를 만족하는 것이 락프리 알고리즘이다(이러한 성격을 가지고 있어야 한다.).
* wait free는 충돌했을 경우 상관없이 끝나면된다.
* lock free는 충돌하면 딜레이가 생길 수는 있으나 다른 나머지 하나는 반드시 딜레이 없이 완료해야한다.
	ex) 100개가 부딪히면 99개는 기다리더라도 적어도 1개의 승자는 딜레이없이 끝나야 lock free이다.
  승자가 없거나 승자가 있더라도 걔가 딜레이 될 수 있다면 lock free가 아니다.

- wait free 알고리즘이란?
1) 호출이 다른 쓰레드와 충돌해도 모두 delay 없이 완료된다.
   (모두 다 정해진 시간에 끝나면 wait free이다.)

- 추가 상식
1) lock을 사용하지 않는다고 lock free 알고리즘이 아니다. ex) while(dataReady == false) 이런 식으로 기다리는 것
2) lock을 사용하면 무조건 lock free 알고리즘이 아니다.

						알고리즘
					 ↙          ↘
	     싱글쓰레드 전용       멀티쓰레드
		(stl 컨테이너들)	   ↙	   ↘
						 Blocking	   Non-blocking
						               ↙        ↘
								  Lock-free		  ...
								  ↙      ↘
							 Wait-free     ...

ex) 
push(int x)
{
	Node* n = new Node(x);
	lock.lock();
	tail->next = n;
	tail = n;
	lock.unlock();
}
을 논블로킹으로 하면?

push(int x)
{
	Node* n = new Node(x);
	while(true)
	{
		Node* last = tail;
		Node* next = last->next;
		if(last != tail) continue;
		if(next == NULL)
		{
			if(CAS(&(last->next), NULL, n))
			{
				CAS(&tail, last, n);
				return;
			}
		}
		else CAS(&tail, last, next);
	}
}
가 된다. 뮤텓스를 걸지 않는다. 다른 쓰레드가 뭘 하든 계속 돌아간다. wait-free이다.
CAS -> lock free 알고리즘을 구현하기 위해서 필수다.

- CAS
1) CAS 없이는 대부분의 non blocking 알고리즘들을 구현할 수 없다.
	ex) queue, stack, list
2) CAS를 사용하면 모든 싱글쓰레드 알고리즘들을 lock free 알고리즘으로 변환할 수 있다. (중요)
3) lock free 알고리즘의 핵심
	cas(&a, old, new);
	의미 : a의 값이 old면 new로 바꾸고 true를 리턴
	다른 버전의 의미 : a 메모리를 다른 쓰레드가 먼저 업데이트 해서 false가 나왔다면 모든 것을 포기하고 다시 시도

lock.lock();				while(true) {
sum = sum + 2;			->	int old_sum = sum;
lock.unlock();				if(CAS(&sum, old_sum, old_sum + 2 )) break; }

CAS를 통해 이렇게 치환 가능하다.

4) CAS 쉬워보인다.
	tail값도 바꿔야하고 next도 바꿔야하고 비교하고 변경 했나 안했나 감시하고 하면서 한방에 업데이트 하면 된다. 하지만 쉽지 않다.
	만약 head -> x -> 3 -> 1 -> 9 <- tail 여기에 push(35)를 한다면
	tail과 tail의 next를 동시에 보면서 바꿔주는, 즉 2개의 메모리를 동시에 보는 듀얼 CAS가 구현된 CPU가 없다.
	그렇기에 CAS 한번에 두 번한 효과를 내야해서 알고리즘이 복잡해진다. 그래서 논블로킹 프로그램이 어렵다.
	실수할 수 있는데 실수했다고 바로 알아차리기 어렵다. 이 실수가 오작동을 일으키지만 자주 오작동하지 않는다.

- 종합
1) 알고리즘이 많이 복잡하다.
2) 그래서 작성시 실수하기 쉽다.
3) 실수를 알아차리기 힘들다.(끔찍하다)
	[1] 하루에 한 두 번 서버 크래시
	[2] 가끔 아이템 증발
4) 제대로 동작하는 것이 증명된 알고리즘을 사용해야한다.
5) 그러므로 믿을 수 있는 non blocking container들을 사용해라
	ex) intel TBB, Visual Studio PPL, c++20, boost
6) 자신을 포함한 출처가 의심스러운 알고리즘은 정확성을 증명하고 사용하라.
	(정확성이 증명된 논문에 있는 알고리즘은 OK)

- lock-free 동기화
1) add, remove는 lock free로. wait free로 구현하면 프로그램이 지저분해진다.
2) Contains()는 이미 wait free

- non blocking 구현이란?
1) 게으른 동기화를 통해 만족할만한 멀티쓰레드 향상을 얻었으나 블로킹 구현이라 성능 향상의 여지가 남아있고
   우선순위 역전이나 Convoying에서 자유롭지 못하다.
2) 논블로킹 구현은 게으른 동기화에서 출발한다. 이미 충분히 최적화 되어있다.
3) 락과 오버헤드의 최소화 그리고 marking을 사용할 것이다. O(1)의 Validation
	[1] 락을 사용하지 않는다.
	[2] 서로 경쟁하는 Thead는 cas로 승부를 낸다.
		(1) 한번의 cas로 승부 결정
		(2) 이겼으면 무조건 메쏘드가 성공적으로 종료해야한다. 
			{1} 적어도 이전보다는 더 진전된 상태로 바뀌어야 한다.
		(3) 패배했으면?
			{1} cas가 실패하면 다른 쓰레드가 먼저 변경을 실행한 것이므로 변경을 포기해야한다.
			{2] cas가 실패했으므로 이 위치에서는 더 이상 작업할 수 없고 다른 위치를 찾아야한다.
			{3} 졌다는 이야기는 다른 쓰레드에서 먼저 자료구조를 수정했다는 이야기이므로,
			    지금까지 수집한 자료구조 정보를 더 이상 사용할 수없으므로 다시 수집해야한다.

- CAS의 한계
1) 한번에 하나의 변수밖에 바꾸지 못한다. -> 꼼수로 해결
	[1] 주소와 마킹이 두 군데 떨어져 있어서 못한다면 합치면 된다.(한 장소에 주소와 마킹을 동시에 저장)
		그러면 메모리 하나에 대해 CAS로 설명할 수 있다.(attemptMark 함수 만들기)
	[2] 그러나 모든 경우에는 안되고 특수한 경우에만 사용 가능하다.(일반적으로는 아무 때나 변수를 합칠 수 없다.)
2) 검색 횟수를 줄이기 위해 marking이 필요하다.
	[1] 게으른 동기화와 비슷한 개념
	[2] 여기서 마킹은 노드의 삭제를 의미한다.
3) marking과 next의 atomic한 동시 변환이가능해야 한다.

- 변형
1) 하나의 변수에 주소와 marking을 동시에 저장
2) marking 변경용 cas 재공 (attemptMark)

- Window에서의 멀티 CAS 구현
1) CAS(oldmark, mark, oldnext, next)
2) 32비트 주소 중 LSB를 마크로 사용(1비트를 mark로 사용)
	[1] next필드를 포인터로 직접 사용할 수 없게 되었으므로, 모든 next 필드를 통한 node이동 시 type 변환이 필요하다.
	[2] 디버깅이 어려워진다.
		(1) 포인터가 아닌 다른 데이터 타입으로 선언해도 된다.
*/

// __QUEUE__

/*
1. __COARSE_GRAINED_SYNCHRONIZATION__ : 성긴 동기화
- POOL
1) 리스트는 Set 객체
2) queue와 stack은 pool 객체
3) pool 객체란?
	[1] 같은 아이템의 복수 존재를 허용
	[2] contains() 메소드를 항상 제공하지 않는다.
	[3] get()과 set() 메소드를 제공한다.
	[4] 보통 생산자 - 소비자 문제의 버퍼로 사용된다.
4) linkedList로 Set을 구현했었다. 그래서 중복이 안됐다. 그래서 있냐 없냐 체크가 중요했다.
add remove할 때마다 contains로 값이 있나 없나 검색했다. -> 검색에 중점을 둔 객체가 set 객체이다.
5) pool도 add remove가 있다. 하지만 검색이 없고 중복이 허용된다. contains 메소드가 있을 순 있으나 이게 필요하면
굳이 pool을 쓸 이유가 없다. 필요하다면 set 객체를 쓰는 것이 좋다. 그래서 pool은 contains 메소드가 있다고 가정하고 만든 객체가 아니다.
6) pool은 임시 저장소다. 넣은 것을 그대로 뺄 수 있다. 넣고 빼는 순서에 따라 큐거나 스택, 해시일 수도 있다. 
보통 생산자 - 소비자 문제의 버퍼로 사용된다.

- POOL의 변형
1) 길이 제한
	[1] 있을 수도 있고 없을 수도 있다.(제한, 무제한 전부 가능)
	[2] 제한된 큐가 구현하기 쉽다.
	[3] 제한된 큐는 생산자와 소비자의 간격을 제한한다.
2) 메소드의 성질
	[1] 완전(total) : 특정 조건을 기다릴 필요가 없을 때(락프리에서 사용)
		비어있는 풀에서 get()을 할 때 실패 코드를 반환
		{1} 장점 - 즉시 끝나므로 쓰레드가 딜레이될 일이 없다.
		{2} 단점 - 실패했을 때 어떻게 할 것인가 일일히 다 프로그래밍 해줘야 한다.
	[2] 부분적(partial) : 특정 조건의 만족을 기다릴 때
		비어있는 풀에서 get()을 할 때 다른 누군가가 set()을 할 때까지 기다림
		{1} 장점 - 실패가 없다.(실패를 가정하고 프로그래밍 하지 않아도 된다.
		{2} 단점 - 무한 루프에 빠질 수 있다.
	[3] 동기적(synchronous) : 부분적 큐와 유사하다.
		다른 쓰레드의 메소드 호출의 중첩을 필요로 할때
		랑데부(rendezvous)라고도 부름
		버퍼의 크기가 제로, 
		deq하면 enq쓰레드가 와서 할 때까지 기다린다. 데이터를 enq하는 쓰레드에서 deq하는 쓰레드로 옮기고 끝낸다.(미리 데이터받아놓지 않고 서로 만나서 종료)
		{1} 장점 - 메모리를 쓰지 않는다.
				   여러 쓰레드가 동시에 한 번에 쌍을 이뤄서 끝낼 수 있다.
*/

/*
2. __LOCK_FREE__ : 무제한 무잠금 큐
1) 무잠금(Lock Free)
	[1] CAS를 사용
	[2] 다른 쓰레드가 임의의 위치에 멈춰있어도 진행 보장

동작들 관련은 queue.cpp에 직접 작성
*/