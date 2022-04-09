//
// Created by Eli Levinkopf on 01/04/2022.
//
//#include"uthreads.h"
#include "thread.h"

queue<thread> ready;
thread running;
std::map<int, thread> threads;
thread_entry_point main_entry_point;
int get_next_available_id ();
void setup_timer (int quantum_usecs);
void remove_thread();


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
  timer.it_value.tv_sec =  0;
  timer.it_value.tv_usec = quantum_usecs;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = quantum_usecs;
  // Start a virtual timer. It counts down whenever this process is executing.
  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
	{
	  std::cerr << ERROR << SET_TIMER_ERROR << std::endl;
	  exit (EXIT_FAILURE);
	}
}

int uthread_terminate(int tid){
  if (threads.find (tid) == threads.end() || threads.find (tid)->second.get_state() == TERMINATED){
	return FAILURE;
  }
  if (!tid){
	for (auto &it: threads){
	  it.second.free();
	}
	exit (EXIT_SUCCESS);
  }
  if (running.get_id() == tid){
	  threads[tid].free();
	  threads[tid].set_state(TERMINATED);
	  timer_handler(SIGVTALRM);
	}
  threads[tid].free();
  threads[tid].set_state(TERMINATED);
  remove_thread();
  return SUCCESS;
}

void remove_thread(){
  queue<thread> tmp;
  while(!tmp.empty()){
	thread tmp_thread = ready.front();
	if (tmp_thread.get_state() == READY){
	  tmp.push (tmp_thread);
	}
  }
  ready = tmp;
}



int uthread_spawn (thread_entry_point entry_point)
{
  int id = get_next_available_id();
  thread new_thread = thread (id, entry_point);
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

//int main ()
//{
//  uthread_init (10);
//  uthread_spawn (f);
//  uthread_spawn (f);
//  uthread_spawn (f);
//  uthread_spawn (f);
//
//  int i = 0;
//  while (1)
//  {
//    i++;
//    if (i%10000==0)
//    {
//      std::cout << running.get_id ();
//    }
//
//  }
//
//}

