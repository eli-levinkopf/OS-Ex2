//
// Created by Eli Levinkopf on 04/04/2022.
//

#ifndef _THREAD_H_
#define _THREAD_H_


#include "uthreads.h"

enum state {
  RUNNING, READY, BLOCKED, SLEEPING, TERMINATED
};

class thread {
  int _id;
  char *_stack;
  int _num_of_quantums;

 public:
  enum state _state;
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

  void set_state(enum state s){
	_state = s;
  }

  void free(){
	delete[] _stack;
  }

};

#endif //_THREAD_H_
