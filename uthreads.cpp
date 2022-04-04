//
// Created by Eli Levinkopf on 01/04/2022.
//
#include "uthreads.h"
#include <stdlib.h>
#include <iostream>
#include <map>
#include <sys/time.h>
#include <queue>
#include <vector>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#define SUCCESS 0
#define FAILURE -1
#define ERROR "system error: "
#define SIG_ACTION_ERROR "sigaction error."
#define SET_TIMER_ERROR "setitimer error."
#define TID_ERROR "no thread with ID tid exists"

using std::queue;
using std::vector;

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address (address_t addr)
{
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

int get_next_available_id ();

thread_entry_point main_entry_point;
enum state {
  RUNNING, READY, BLOCKED, SLEEPING, TERMINATED
};

class thread {
  int _id;
  enum state _state;
  char *_stack;
  int _num_of_quantums;

public:
  sigjmp_buf _env;
  thread () = default;
  thread (const int &id, thread_entry_point entry_point = nullptr)
	  : _id (id), _state (READY), _stack (new char[STACK_SIZE]), _num_of_quantums(0)
  {
	if (_id)
	  {
		sigsetjmp (_env, 1);
		auto sp = (address_t)_stack + STACK_SIZE - sizeof (address_t);
		auto pc = (address_t)entry_point;
		(_env->__jmpbuf)[JB_PC] = translate_address (pc);
		(_env->__jmpbuf)[JB_SP] = translate_address (sp);
	  }
	sigemptyset(&_env->__saved_mask);
  }

//  ~thread()
//  {delete[] _stack;}

//  sigjmp_buf* get_env() const
//  {return _env;}

  int get_id () const
  {return _id;}

  int get_num_of_quantums() const
  {return _num_of_quantums;}

  int get_state()const
  {return _state;}

  void set_quantum()
  {++_num_of_quantums;}

};

queue<thread> ready;
//std::vector<std::pair<thread, bool>> threads;
int quantum;
struct itimerval timer;
void setup_timer (int quantum_usecs);
thread running;
int total_quantums = 0;
std::map<int, thread> threads;


void timer_handler (int sig)
{
  if (!ready.empty ())
	{
	  int ret_val = sigsetjmp (running._env, 1);
	  if (ret_val)
		{
		  threads[running.get_id ()].set_quantum();
		  ++total_quantums;
		  return;
		}
	  ready.push (running);
	  running = ready.front ();
	  ready.pop ();
	  threads[running.get_id ()].set_quantum();
	  ++total_quantums;
	  siglongjmp (running._env, 1);
	}
}

int uthread_init (int quantum_usecs)
{
  if (quantum_usecs < 0)
	{return FAILURE;}

  setup_timer (quantum_usecs);
  thread main = thread (0);
//  std::pair<thread, bool> pair (main, true);
  threads[0] = main;
  running = main;
  quantum = quantum_usecs;
  return SUCCESS;
}

void setup_timer (int quantum_usecs)
{
  struct sigaction sa = {nullptr};
  sa.sa_handler = &timer_handler;
  if (sigaction (SIGVTALRM, &sa, nullptr) < 0)
	{
	  std::cerr << ERROR << SIG_ACTION_ERROR << std::endl;
	  exit (EXIT_FAILURE);
	}
  timer.it_value.tv_sec = quantum_usecs / 1000000;
  timer.it_value.tv_usec = quantum_usecs % 1000000;
  timer.it_interval.tv_sec = quantum_usecs / 1000000;
  timer.it_interval.tv_usec = quantum_usecs % 1000000;
  // Start a virtual timer. It counts down whenever this process is executing.
  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
	{
	  std::cerr << ERROR << SET_TIMER_ERROR << std::endl;
	  exit (EXIT_FAILURE);
	}
}

int uthread_spawn (thread_entry_point entry_point)
{
  int id = get_next_available_id();
//  std::cout<<id<<std::endl;
  thread new_thread = thread (id, entry_point);
//  std::pair<thread, bool> pair(new_thread, true);
  threads[id] = new_thread;
  ready.push (new_thread);
  return SUCCESS;
}

int uthread_get_tid()
	{return running.get_id();}


int uthread_get_total_quantums()
	{return total_quantums;}

int uthread_get_quantums(int tid){
  if (threads.find(tid) == threads.end()){
	std::cerr << ERROR << TID_ERROR << std::endl;
	exit (EXIT_FAILURE);
  }
  return threads[tid].get_num_of_quantums();
}

int get_next_available_id (){
  int active_threads = 0;
  for (auto const &it: threads)
	{
	  if (it.second.get_state() != TERMINATED)
		{
		  active_threads++;
		}
	  else {break;}
	}
  if (active_threads >= MAX_THREAD_NUM)
	{return FAILURE;}

  return active_threads;
}

void f ()
{
  int i = 0;
  while (1)
	{
	  i++;
	  if (i % 10000 == 0)
		{
		  std::cout << running.get_id ();
		}
	}
}

int main ()
{
  uthread_init (10);
  uthread_spawn (f);
  uthread_spawn (f);
  uthread_spawn (f);
  uthread_spawn (f);

  int i = 0;
  while (1)
  {
    i++;
    if (i%10000==0)
    {
      std::cout << running.get_id ();
    }

  }

}

