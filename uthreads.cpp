//
// Created by Eli Levinkopf on 01/04/2022.
//
#include "uthreads.h"
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include <queue>
#include <vector>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
using std::queue;
using std::vector;
#define SUCCESS 0
#define FAILURE -1
#define ERROR "system error: "
#define SIG_ACTION_ERROR "sigaction error."
#define SET_TIMER_ERROR "setitimer error."

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr) {
  address_t ret;
  asm volatile("xor    %%fs:0x30,%0\n"
               "rol    $0x11,%0\n"
  : "=g" (ret)
  : "0" (addr));
  return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}


#endif

thread_entry_point main_entry_point;
enum state {
  RUNNING, READY, BLOCKED, SLEEPING
};

class thread {
  int _id;
//  sigjmp_buf _env;
  enum state _state;
  char* _stack;
 public:
  sigjmp_buf _env;
  thread() = default;
  thread(int id, thread_entry_point entry_point)
      : _id(id), _state(READY), _stack(new char[STACK_SIZE]) {
    auto sp = (address_t) _stack + STACK_SIZE - sizeof(address_t);
    auto pc = (address_t) entry_point;
    sigsetjmp(_env, 1);
    (_env->__jmpbuf)[JB_SP] = translate_address(sp);
    (_env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&_env->__saved_mask);
  }



//  ~thread() {
//    delete[] _stack;
//  }

//  sigjmp_buf* get_env() const {
//    return _env;
//  }

  int get_id() const {
    return _id;
  }
};

queue<thread> ready;
std::vector<std::pair<thread, bool>> threads;
int quantum;
struct itimerval timer;
thread running;

void timer_handler(int sig) {
  if (!ready.empty()) {
    //TODO sp and pc are correct?
    int ret_val = sigsetjmp(running._env, 1);
    std::cout << (running.get_id() == threads[ret_val].first.get_id());
    ready.push(running);
    running = ready.front();
    ready.pop();
    siglongjmp(running._env, 1);
  }
  else{
    std::cout<<"====="<<std::endl;
  }
}

int uthread_init(int quantum_usecs) {
  if (quantum_usecs < 0) {
    return FAILURE;
  }
  struct sigaction sa = {nullptr};
  sa.sa_handler = &timer_handler;
  if (sigaction(SIGVTALRM, &sa, nullptr) < 0) {
    std::cerr << ERROR << SIG_ACTION_ERROR << std::endl;
    exit(EXIT_FAILURE);
  }
  thread main = thread(0, main_entry_point);
  std::pair<thread, bool> pair(main, true);
  threads.push_back(pair);
  ready.push(main);
  // Configure the timer to expire after 1 sec... */
  timer.it_value.tv_sec = 1;        // first time interval, seconds part
  timer.it_value.tv_usec = 0;        // first time interval, microseconds part

  // configure the timer to expire every 3 sec after that.
  timer.it_interval.tv_sec = 0;    // following time intervals, seconds part
  timer.it_interval.tv_usec =
      quantum_usecs;    // following time intervals, microseconds part

  // Start a virtual timer. It counts down whenever this process is executing.
  if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
    std::cerr << ERROR << SET_TIMER_ERROR << std::endl;
    exit(EXIT_FAILURE);
  }
  quantum = quantum_usecs;
  return SUCCESS;
}

int uthread_spawn(thread_entry_point entry_point) {
  int active_threads = 0;
  for (auto pair: threads) {
    if (pair.second) {
      active_threads++;
    } else {
      break;
    }
  }
  if (active_threads >= MAX_THREAD_NUM) {
    return FAILURE;
  }
  thread new_thread = thread(active_threads, entry_point);
  std::pair<thread, bool> pair(new_thread, true);
  threads[active_threads] = pair;
  ready.push(new_thread);
  return SUCCESS;
}

int main(){
  uthread_init (6);
}