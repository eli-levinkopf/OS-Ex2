//
// Created by Eli Levinkopf on 04/04/2022.
//
#include "uthreads.h"

#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <csetjmp>
#include <map>
#include <sys/time.h>
#include <deque>
#include <vector>
#include <stdio.h>
#include <set>
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



//thread_entry_point main_entry_point;
using std::deque;
using std::map;

int quantum;
struct itimerval timer;
int total_quantums = 1;


enum state {
  RUNNING, READY, BLOCKED, SLEEPING, TERMINATED
};

class thread {
  int _id;
  char *_stack;
  int _num_of_quantums;
  int _sleep_time;

 public:
  enum state _state;
  sigjmp_buf _env;
  thread(): _id(0), _stack(nullptr), _state(RUNNING), _num_of_quantums(1) {
      sigemptyset(&_env->__saved_mask);
  }
  thread (const int &id, thread_entry_point entry_point)
	  : _id (id), _state (READY), _stack (new char[STACK_SIZE]), _num_of_quantums(0), _sleep_time(0)
  {
    sigsetjmp (_env, 1);
    auto sp = (address_t)_stack + STACK_SIZE - sizeof (address_t);
    auto pc = (address_t)entry_point;
    (_env->__jmpbuf)[JB_PC] = translate_address (pc);
    (_env->__jmpbuf)[JB_SP] = translate_address (sp);
	sigemptyset(&_env->__saved_mask);
  }


  ~thread()
  {
      delete[] _stack;
      _stack = nullptr;
  }


  int get_id () const
  {return _id;}

  int get_num_of_quantums() const
  {return _num_of_quantums;}

  int get_state()const
  {return _state;}

  void set_quantum()
  {++_num_of_quantums;}

  void set_state(enum state s){
	_state = s;
  }

  void set_sleep_time(int sleep_time) {
      _sleep_time = sleep_time;
  }

  void decrease_sleep_time() {
      _sleep_time--;
  }

  int get_sleep_time() const {
      return _sleep_time;
  }

  void free(){
	delete[] _stack;
	_stack = nullptr;
  }

};

#endif //_THREAD_H_
