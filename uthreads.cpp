//
// Created by Eli Levinkopf on 01/04/2022.
//
//#include"uthreads.h"
#include "thread.h"

queue<thread> ready;
int running_id;
std::map<int, thread> threads;
thread_entry_point main_entry_point;
sigset_t set;
int get_next_available_id ();
void setup_timer (int quantum_usecs);
void remove_thread ();
void block_signals();
void unblock_signals();


void timer_handler (int sig)
{
    if (!ready.empty ())
    {
        if (threads[running_id].get_state () != TERMINATED)
        {
            int ret_val = sigsetjmp (threads[running_id]._env, 1);
            if (ret_val)
            {
                threads[running_id].set_quantum ();
                ++total_quantums;
                return;
            }
            ready.push (threads[running_id]);
        }
        running_id = ready.front ().get_id();
        ready.pop ();
        threads[running_id].set_quantum ();
        ++total_quantums;
        siglongjmp (threads[running_id]._env, 1);
    }
}

int uthread_init (int quantum_usecs)
{
  if (quantum_usecs < 0)
	{ return FAILURE; }

  setup_timer (quantum_usecs);
  running_id = 0;
  thread main = thread (running_id);
  threads[0] = main;
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
  timer.it_value.tv_sec = 0;
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

int uthread_terminate (int tid)
{
  block_signals();
  if (threads.find (tid) == threads.end () || threads.find (tid)->second.get_state () == TERMINATED)
	{
	  unblock_signals();
	  return FAILURE;
	}
  if (!tid)
	{
	  for (auto &it: threads)
		{
	      if (it.second.get_state() != TERMINATED) {
	          it.second.free ();
	      }
		}
	  exit (EXIT_SUCCESS);
	}
  if (running_id == tid)
	{
	  threads[tid].free ();
	  threads[tid].set_state (TERMINATED);
//	  remove_thread ();
	  timer_handler (SIGVTALRM);
	}
  else
	{
	  threads[tid].free ();
	  threads[tid].set_state (TERMINATED);
	  remove_thread ();
	  unblock_signals();
	  return SUCCESS;
	}
	unblock_signals();
}

void remove_thread ()
{
  queue<thread> tmp;
  while (!ready.empty ())
	{
	  thread tmp_thread = ready.front ();
	  ready.pop();
	  if (tmp_thread.get_state () == READY)
		{
		  tmp.push (tmp_thread);
		}

	}
  ready = tmp;
}

int uthread_spawn (thread_entry_point entry_point)
{
  block_signals();
  int id = get_next_available_id ();
  thread new_thread = thread (id, entry_point);
  threads[id] = new_thread;
  ready.push (new_thread);
  unblock_signals();
  return SUCCESS;
}

int uthread_get_tid ()
{ return running_id; }

int uthread_get_total_quantums ()
{ return total_quantums; }

int uthread_get_quantums (int tid)
{
  block_signals();
  if (threads.find (tid) == threads.end ())
	{
	  std::cerr << ERROR << TID_ERROR << std::endl;
	  exit (EXIT_FAILURE);
	}
	unblock_signals();
  return threads[tid].get_num_of_quantums ();
}

int get_next_available_id ()
{
  int active_threads = 0;
  for (auto const &it: threads)
	{
	  if (it.second.get_state () != TERMINATED)
		{
		  active_threads++;
		}
	  else
		{ break; }
	}
  if (active_threads >= MAX_THREAD_NUM)
	{ return FAILURE; }

  return active_threads;
}

void block_signals(){
  sigemptyset(&set);
  sigaddset(&set, SIGALRM);
  sigprocmask(SIG_BLOCK, &set, nullptr);
}

void unblock_signals(){
  sigprocmask(SIG_UNBLOCK, &set, nullptr);
}

void f ()
{
  int i = 0;
  while (1)
	{
	  i++;
	  if (i % 10000 == 0)
		{
		  std::cout << running_id;
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

