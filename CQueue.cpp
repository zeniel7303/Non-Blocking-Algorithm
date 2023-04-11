#include "CQueue.h"

#ifdef __QUEUE__
#ifdef __COARSE_GRAINED_SYNCHRONIZATION__
CQueue::CQueue()
{
	head = tail = new CNode(0);
}
CQueue::~CQueue() {}

void CQueue::Init()
{
	// 알고리즘 간단화를 위해 보초노드 하나 두기
	CNode* ptr;
	while (head->next != nullptr) {
		ptr = head->next;
		head->next = head->next->next;
		delete ptr;
	}
	tail = head;
}

void CQueue::Enq(int key)
{
	CNode* e = new CNode(key);
	glock.lock();
	tail->next = e;
	tail = e;
	glock.unlock();
}

int CQueue::Deq()
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

void CQueue::Display20()
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
#endif
#ifdef __LOCK_FREE__
CLockFreeQueue::CLockFreeQueue()
{
	m_head = m_tail = new CNode(0);
}

CLockFreeQueue::~CLockFreeQueue()
{

}

void CLockFreeQueue::Init()
{
	CNode* ptr;
	while (m_head->m_next != nullptr)
	{
		ptr = m_head->m_next;
		m_head->m_next = m_head->m_next->m_next;
		delete ptr;
	}
	m_tail = m_head;
}

bool CLockFreeQueue::CAS(CNode* volatile* _addr, CNode* _oldNode, CNode* _newNode)
{
	return atomic_compare_exchange_strong(
		reinterpret_cast<volatile atomic_int*>(_addr),
		reinterpret_cast<int*>(&_oldNode),
		reinterpret_cast<int>(_newNode));
}

/*
- Enqueue의 기본 동작
1) Tail이 가리키는 node에 CAS로 새 노드를 추가
2) 실패하면 Next로 이동한 후 추가
3) 성공하면 Tail을 이동
*/
void CLockFreeQueue::Enq(int _key)
{
	/*
	직관적인 구현
	성공하면 tail을 밀어넣고 리턴, 실패하면 처음부터 다시

	CNode* temp = new CNode(_key);
	while (true)
	{
		CNode* last = m_tail;

		// tail이 가리키는 것의 next가 NULL이면 temp를 넣자.
		if (CAS(&last->m_next, NULL, temp))
		{
			// 성공
			// tail을 이동(밀어넣기)
			//m_tail = temp;

			// 하지만 위에껀 문제가 발생한다.
			// 다른 쓰레드가 와서 내가 성공한 다음 계속 쌓아놓았다. 그럼 계속 전진하다가 뒤늦게 덮어쓰면?
			// 쭉 뒤로 갔던 tail을 앞으로 쭉 땡겨오게 된다. 이 상황에서 deq가 돼서 정보가 날아가면 끝이다.
			CAS(&m_tail, last, temp)
			// 이렇게 하면? 또 큰 문제가 발생한다.
			// 누군가 첫번째 CAS를 성공하고 두번째 CAS 진행중에 다른 쓰레드가 enq하려고 CAS를 하게되면 이 CAS는 무조건 실패한다.(tail의 next가 null이 아니므로)
			// 이건 논블로킹 프로그램이 아니다.
			// 먼저 온 쓰레드의 2번째 CAS가 성공될때까지 뒤에 온 쓰레드는 계속 기다려야한다.
			return;
		}
		// 실패(누가 먼저 새치기해서 끼워넣음)
		// 처음부터 다시
	}
	*/

	/*
	위의 해결책 -> tail의 전진이 모든 쓰레드에서 가능하게 한다.
	linkedList에서 CAS 두 번 할때 자료구조가 이상해졌다. 이상한 자료구조도 허용하게 했었다. 이 곳도 마찬가지다.
	tail 노드 끝에 붙였는데 tail 포인터가 전진하지 못했을때도 알고리즘이 정장적으로 동작하도록 만들자
	성공하면 전진, 실패하면 계속 루프돌게 아니라 그냥 넘어가고 다른 쓰레드가 tail을 전진시키자.
	CAS(&m_tail, last, next);
	이 방법으로, 그리고 CNode* last = m_tail로 돌아가기.
	*/
	CNode* temp = new CNode(_key);
	while (true)
	{
		// tail을 last로 복사해 사용 중인데 이 노드가 계속 멀쩡하게 있으리라는 보장이 없다.
		// 재사용되면 날아가버릴 수 있다. 그러므로 next까지 미리 읽어둔다.
		// last와 lastNext를 미리 읽어두고 이 next가 제대로 된 next인지 확인한다. 내가 읽은 tail 값에서 변경되지 않았다면
		// next를 제대로 읽은 것이고 아니면 그 사이에 또 tail이 바뀌었다는 것은 충돌한 것이니까 처음부터 다시 한다.
		CNode* last = m_tail;
		CNode* next = last->m_next;

		if (last != m_tail) continue;
		// 보통 next는 nullptr이다. null이 아니면 다른 쓰레드가 끼어들어 tail을 집어넣었고 전진이 아직 안된 것이다.
		if (next != nullptr)
		{
			// tail을 대신 전진시켜줘야한다.
			CAS(&m_tail, last, next);
			continue;
		}

		// next가 nullptr이니까 다른 쓰레드가 끼어들지 않았으므로 next를 바꾸고
		if (CAS(&last->m_next, nullptr, temp) == false) continue;

		// 전진시킨다.(성공하든 말든)
		CAS(&m_tail, last, temp);

		return;
	}
}

int CLockFreeQueue::Deq()
{
	/*
	// 1차 구현
	// deq는 head에서 꺼내는 것이다. 값을 리턴하고 head를 전진해야한다.
	// 당연히 전진은 CAS로 해야한다.
	while(true)
	{
		CNode* first = m_head;
		if (!CAS(&m_head, first, first->m_next)) continue;
		// 성공하면 값을 읽어 리턴
		int value = first->m_next->m_key;
		delete first;
		return value;
	}
	// 이렇게 읽어도 되는가? 가능하다.
	// first로 했을 때 first->next는 'a'인데 다른 쓰레드가 first를 지울 확률은? 없다. first는 나만 갖고있다.
	// 그러면 first를 잘 갖고있으니 이 next를 그냥 읽으면 되나? 그건 안된다.
	// 제대로 되어있는 값을 다른 쓰레드가 건드는건 아니지만 문제는 'a'이다. 노드는 다른 쓰레드가 delete를 통해 날릴 수 있다.
	*/

	/*
	// 2차 구현
	while (true)
	{
		CNode* first = m_head;
		// next를 추가했다.
		// first가 바뀌었나 안바뀌었나 head와 같은지 보고, 바뀌었다면 next가 제대로된 값인지 확신할 수 없다.
		// next가 제대로 된 값인지 아닌지는, head가 first 값을 유지하고 있나 아닌가를 보면 된다.
		// 유지하고 있다면 제대로 된 값이 유지되고 있는 상태이므로 이때 value를 읽는다.
		CNode* next = first->m_next;
		if (first != m_head) continue;
		// 큐가 Empty인 지도 확인해야한다. first 즉 head의 next가 null이라면 비어있는 큐다.(보초 노드만 있다.) 그러면 그냥 끝낸다.
		// 완전 큐이므로 해당 경우는 프로그램이 죽던가 에러코드를 내던가 해야한다.
		if (first->m_next == NULL) return EmptyError();
		int value = next->m_key;
		if (CAS(&m_head, first, next)) continue;
		delete first;
		return value;
	}
	
	프로그램이 긴 이유는 헤드 값을 미리 저장하고 그 값이 바뀌는지 사전검사가 있어서 그렇다. 
	핵심은 두줄이다. value를 읽고 CAS하는 것. 실패하면 루프, 성공하면 리턴이다.
	근데 이게 끝이 아니다. 구멍이 여전히 하나 존재한다. 예외 케이스가 있다.
	- int value = first->next->key
		다른 쓰레드에서 first를 꺼내 free 시키면 어떤 일이 벌어질지 알 수 없다.
		Segment Fault의 가능성이 있다.
	- Tail과 Head가 만나는 경우
		Tail이 enq하는 동안 deq가 일어나 tail이 삭제된 보초 노드를 가리키는 순간이 생긴다.
	enq와 deq가 따로 놀면 상관이 없다. 하지만 큐가 비어있을 경우 enq, deq가 동시에 호출되면 서로 꼬인다. 그 경우도 검사해야한다.
	deq에 경우 헤드는 CAS 한번으로 끝난다. 하지만 enq는 CAS를 2번하고 첫번째 CAS가 성공하고 두번째 CAS를 하지 않고 있는 그 짧은 순간에 
	큐가 이상하게 꼬이는 순간이 존재한다.

	(리소스 파일 다운로드.png 참고)
	Tail이 보초노드를 가리키고있다면 큐가 비어있는 상태다. 그때 다른 쓰레드가 와서 enq를 한다.
	enq를 하면 새 노드를 보초 노드에 연결하고 tail을 전진시키는게 순서다. 
	하지만 문제는 enq하는 쓰레드가 보초 노드에 새로운 노드 b를 그림과 같이 연결을 해놨다.
	연결까진 OK지만 tail을 전진시키지 못하고 있을 때 다른 쓰레드가 deq를 하는 것이다. deq는 head만 본다.
	head의 next가 a를 가리키고 있고 first의 값을 head에 넣고n ext가 a가 있으니 nullptr이 아니다.
	그러면 연결되어있으니(큐에 데이터가 있으니) deq를 한다. deq에서 CAS를 한다. head를 전진시킨다. 이후 tail 노드는 delete 하게 된다.
	A가 first이고 head를 전진 시켰으니 성공적으로 deq를 하고 a를 삭제한다.그럼 tail은 delete된 값을 가리키게 된다.
	아직 b를 가리키지 못하고 전진을 못하고 있는데 tail이 가리키는 노드가 날라가버렸다. 그럼 다른 쓰레드가 와서 enq를 할 떄
	null이니까 전진해서 붙여야하는데 delete되어 뭔 값인지 알 수가 없어 프로그램이 터진다.

	이렇게 tail이 아직 전진하지 못했는데 deq 하면 안된다.head하고 tail이 같은 경우 deq 하지 않는 것도 말이 안된다.
	위와 같은 경우일 수도 있지만 정말 enq되어 있을 수도 있다. 그러면 b 데이터를 리턴해줘야하는데 어떻게 하나?
	head와 tail이 같은데 empty가 아니다? 그럼 tail을 전진시키고 해주면된다. 그래서 enq에서 tail의 전진을 보조해주는 코드가 필요하다.
	*/

	//그것이 이 코드다.
	while (true)
	{
		//head와 tail 전부 읽어 둔다.
		CNode* first = m_head;
		CNode* firstNext = first->m_next;
		CNode* last = m_tail;
		CNode* lastNext = last->m_next;
		if (first != m_head) continue;
		// first와 last가 같다.
		if (last == first)
		{
			// last의 next가 nullptr이면 뭐 어쩔수 없다. 정말 비어있는 것이다.
			if (lastNext == nullptr)
			{
				return EmptyError();
			}
			// 근데 nullptr이 아니다? 새 값은 왔는데 tail만 전진하지 못한 경우다.
			else
			{
				// tail을 전진시켜준다.
				CAS(&m_tail, last, lastNext);
				// 그리고 처음부터 다시
				// 그대로 진행하려하면 정말 많은 경우를 따져야한다. 차라리 처음부터 다시 하는게 속편하다.
				continue;
			}
		}
		if (firstNext == nullptr) continue;
		int result = firstNext->m_key;
		if (CAS(&m_head, first, firstNext) == false) continue;
		first->m_next = nullptr;
		// 오동작할 수 있으니 보류(ABA 문제에서 다룰 것)
		//delete first;
		return result;
	}
	// 이러면 이제 tail이 delete된 노드를 가리키는 경우가 생기지 않는다. 이것이 마지막 버전이다.
	// 프로그램이 터질일도 없고 모든 경우에서 멈추지 않고 동작한다.
	// 그리고 주의점이 있다. (컴파일러 최적화 문제)
	// head를 읽고 head를 비교한다.
	// 싱글쓰레드에서는 의미가 없다. 하지만 멀티쓰레드에서는 아니다. 큰 의미가 있다.
	// 컴파일러는 읽은걸 또 읽는다고 무조건 false한다. 무조건 같다고 생각하고 컴파일을 시도하지도 않는다.
	// 그래서 멀티쓰레드에서 돌아가게 하기 위해 head를 volatile로 선언해준다.

	//CNode* volatile m_head;
	//CNode* volatile m_tail;

	//그리고 volatile의 순서도 중요하다. head와 tail이 가리키는 노드가 volatile이 아니라 주소 자체를 volatile로 잡아야 한다.
}

int CLockFreeQueue::EmptyError()
{
	cout << "EMPTY \n";
	this_thread::sleep_for(1ms);
	return -1;
}

void CLockFreeQueue::Display20()
{
	int c = 20;
	CNode* p = m_head->m_next;
	while (p != nullptr)
	{
		cout << p->m_key << ", ";
		p = p->m_next;
		c--;
		if (c == 0) break;
	}
	cout << endl;
}
#endif
#endif