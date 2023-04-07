#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <queue>

#include <Windows.h>

using namespace std;
using namespace chrono;

//#define __LINKEDLIST__
#define __QUEUE__

#ifdef __LINKEDLIST__
//#define __COARSE_GRAINED_SYNCHRONIZATION__
//#define __FINE_GRAINED_SYNCHRONIZATION__
//#define __OPTIMISTIC_SYNCHRONIZATION__
//#define __LAZY_SYNCHRONIZATION__
//#define __Shared_Ptr__
#define __NONBLOCKING_SYNCHRONIZATION__
#else
#define __COARSE_GRAINED_SYNCHRONIZATION__
#endif