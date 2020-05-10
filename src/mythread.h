#include<iostream>
#include<tuple>


#include"defines.h"

#ifndef MY_THREAD_H
#define MY_THREAD_H
template<typename F, typename Tuple, bool Enough, int TotalArgs, int... N>
struct call_impl
{
	auto static call(F f, Tuple t)
	{
		return call_impl<F, Tuple, TotalArgs == 1 + sizeof...(N),
			TotalArgs, N..., sizeof...(N)
		>::call(f, (t));
	}
};


template<typename F, typename Tuple, int TotalArgs, int... N>
struct call_impl<F, Tuple, true, TotalArgs, N...>
{
    auto static call(F f, Tuple t)
    {
        return f(std::get<N>((t))...);
    }
};



    template<typename F, typename Tuple>
auto call(F f, Tuple t)
{
    typedef typename std::decay<Tuple>::type type;
    return call_impl<F, Tuple, 0 == std::tuple_size<type>::value,
           std::tuple_size<type>::value
               >::call(f, (t));
}



template<class retT, class ...argT>
class Thread
{
    public:
        explicit Thread(retT(*f)(argT...), argT...args) {
            _f = f;
            _tu_args = std::tuple<argT...>(args...);
#ifdef WIN_OS
            _handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)_Launch, this, CREATE_SUSPENDED, &_thread_id);
            if (_handle == NULL)
                print_error("CreateThread failed ");
#elif defined(LIN_OS)
            if(pthread_create(&_thread_id, nullptr, (void*(*)(void*))_Launch, this) == -1)
                print_error("pthread_create failed ");
#endif
        }

        void start() {
#ifdef WIN_OS
            if (ResumeThread(_handle) == -1) {
                print_error("ResumeThread failed ");
                return;
            }
            _thread_id = GetCurrentThreadId();
#endif
        }

        void detach() {
#ifdef LIN_OS
            if (pthread_detach(_thread_id) != 0)
                print_error("pthread_detach failed ");
#endif
        }

        void join() {
#ifdef WIN_OS
            if (WaitForSingleObject(_handle, INFINITE) == WAIT_FAILED)
                print_error("WaitForSingleObject(_handle) failed ");
#elif defined(LIN_OS)
            int status = pthread_join(_thread_id, nullptr);
            if (status != 0)
                print_error("pthread_join failed ");
#endif
        }

#ifdef WIN_OS
        DWORD
#elif defined(LIN_OS)
            pthread_t
#endif
            get_id(){ return _thread_id; }




        void cancel() {
#ifdef WIN_OS
            if (TerminateThread(_handle, 1) == NULL)
                print_error("TerminateThread failed ");
#elif defined LIN_OS
            if (pthread_cancel(_thread_id) != 0)
                print_error("pthread_cancel failed ");
#endif
            _finished = true;
        }


        ~Thread()
        {
            if (!_finished)
                cancel();
#ifdef WIN_OS
            if (WaitForSingleObject(_handle, 0) == WAIT_OBJECT_0) {
                if (CloseHandle(_handle) == NULL)
                    print_error("CloseHandle failed ");
                return;
            }
            if (CloseHandle(_handle) == NULL)
                print_error("CloseHandle failed ");
#elif defined(LIN_OS)
            //TODO 
#endif
        }

        bool isFinished() { return _finished; }

    private:
        bool _finished;
        retT(*_f)(argT...);
        std::tuple<argT...> _tu_args;
#ifdef WIN_OS
        HANDLE _handle;
        DWORD 
#elif defined(LIN_OS)
            pthread_t
#endif 
            _thread_id;

        Thread(const Thread&);

        const Thread& operator=(const Thread&) = delete;

        static
#ifdef WIN_OS
            DWORD
#elif defined(LIN_OS)
            void
#endif
            _Launch(void* void_ptr){

                Thread *ptr = (Thread*)void_ptr;
                call<retT(*)(argT...), std::tuple<argT...>>(ptr->_f, ptr->_tu_args);
                ptr->_finished = true;
                //exitThread();
#ifdef WIN_OS
                exitThread(0);
                return 0;
#elif defined(LIN_OS)
                pthread_exit(0);
#endif
            }
};



#endif // MY_THREAD_H
