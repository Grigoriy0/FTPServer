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
        if(pthread_create(&_thread_id, nullptr, (void*(*)(void*))_Launch, this) == -1)
            print_error("E: pthread_create failed ");
    }

    void detach() {
        if (pthread_detach(_thread_id) != 0)
            print_error("E: pthread_detach failed ");
    }

    void join() {
        int status = pthread_join(_thread_id, nullptr);
        if (status != 0)
            print_error("E: pthread_join failed ");
    }

    pthread_t get_id(){ 
        return _thread_id; 
    }

    void cancel() {
        if (pthread_cancel(_thread_id) != 0)
            print_error("E: pthread_cancel failed ");
        _finished = true;
    }


    ~Thread()
    {
        if (!_finished)
            cancel();
    }

    bool isFinished() { return _finished; }

private:
    bool _finished;
    retT(*_f)(argT...);
    std::tuple<argT...> _tu_args;

    pthread_t _thread_id;

    Thread(const Thread&);

    const Thread& operator=(const Thread&) = delete;

    static void _Launch(void* void_ptr){
        Thread *ptr = (Thread*)void_ptr;
        call<retT(*)(argT...), std::tuple<argT...>>(ptr->_f, ptr->_tu_args);
        ptr->_finished = true;
        pthread_exit(0);    
    }
};



#endif // MY_THREAD_H
