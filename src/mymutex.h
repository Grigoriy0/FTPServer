#include<string>

#include"mythread.h"

#ifndef MY_MUTEX_H
#define MY_MUTEX_H

class Mutex
{
public:
	Mutex(const std::string &name) {
#if defined(WIN_OS)
		if ((_handle = CreateMutex(NULL, true, name.c_str())) == 0) {
			print_error("CreateMutex failed ");
		}
#elif defined(LIN_OS)
        if (pthread_mutex_init(&_mu, nullptr) != 0){
            print_error("pthread_mutex_init failed ");
        }
#endif
	}

	void wait() {
#if defined(WIN_OS)
        if (WaitForSingleObject(_handle, INFINITE) == WAIT_FAILED) {
			print_error("WaitForSingleOnject failed ");
		}
#elif defined(LIN_OS)
        if (pthread_mutex_lock(&_mu) != 0){
            print_error("pthread_mutex_lock failed ");
        }
#endif
	}

	void release() {
#if defined(WIN_OS)
        if (ReleaseMutex(_handle) == NULL) {
			print_error("ReleaseMutex failed ");
		}
#elif defined(LIN_OS)
        if (pthread_mutex_lock(&_mu) != 0){
            print_error("pthread_mutex_lock failed ");
        }
#endif
	}

	~Mutex() {
#if defined(WIN_OS)
		if (CloseHandle(_handle) == NULL) {
			print_error("CloseHandle failed ");
		}
#elif defined(LIN_OS)
        if (pthread_mutex_destroy(&_mu) != 0){
            print_error("pthread_mutex_destroy failed ");
        }
#endif
	}
private:
#ifdef WIN_OS
	HANDLE _handle;
#elif defined(LIN_OS)
    pthread_mutex_t _mu;
#endif
};



class MutexLocker {
public:
	MutexLocker(Mutex*mu) { _mu = mu; _mu->wait(); }
	~MutexLocker() { _mu->release(); }
private:
	Mutex*_mu;
};

#endif // MY_MUTEX_H
