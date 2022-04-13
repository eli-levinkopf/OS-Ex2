//
// Created by Eli Levinkopf on 01/04/2022.
//
#include "thread.h"

std::deque<thread*> ready;
int running_id;
std::map<int, thread*> threads;
sigset_t set;
int get_next_available_id();
void terminate_all_threads();
void setup_timer(int quantum_usecs);
void remove_thread();
void block_signals();
void unblock_signals();
void sleep_wake();

void timer_handler(int sig) {
    sleep_wake();
    if (threads[running_id]->get_state() != RUNNING || threads[running_id]->get_state() != READY) {
        if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
            std::cerr << ERROR << SET_TIMER_ERROR << std::endl;
            terminate_all_threads();
            exit(EXIT_FAILURE);
        }
    }
//    std::cout << "running id: " << running_id << " quantum: "<< uthread_get_quantums(running_id) << std::endl;
  if (!ready.empty()) {
	if (threads[running_id]->get_state() != TERMINATED) {
	  int ret_val = sigsetjmp(threads[running_id]->_env, 1);
	  if (ret_val) {
		return;
	  }
	}

	if (threads[running_id]->get_state() == RUNNING) {
	    ready.push_back(threads[running_id]);
	    threads[running_id]->set_state(READY);
	}


	running_id = ready.front()->get_id();
	threads[running_id]->set_state(RUNNING);
	ready.pop_front();
	threads[running_id]->set_quantum();
	++total_quantums;
	int a = running_id;
	siglongjmp(threads[running_id]->_env, 1);
  }
  else {
      threads[running_id]->set_quantum();
      ++total_quantums;
  }
}

void terminate_all_threads() {
    for (auto &it : threads) {
        it.second->free();
        free(it.second);
    }
}

int fail(const std::string& exit_message) {
    std::cerr << "thread library error: " << exit_message << std::endl;
    return FAILURE;
}
int uthread_init(int quantum_usecs) {
  if (quantum_usecs <= 0) {
      return fail("quantum should be positive");
  }

  running_id = 0;
  thread* main = new thread();
  setup_timer(quantum_usecs);
  threads[0] = main;
  quantum = quantum_usecs;
  return SUCCESS;
}

void setup_timer(int quantum_usecs) {
  struct sigaction sa = {nullptr};
  sa.sa_handler = &timer_handler;
  if (sigaction(SIGVTALRM, &sa, nullptr) < 0) {
	std::cerr << ERROR << SIG_ACTION_ERROR << std::endl;
	terminate_all_threads();
	exit(EXIT_FAILURE);
  }
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = quantum_usecs;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = quantum_usecs;
  if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
	std::cerr << ERROR << SET_TIMER_ERROR << std::endl;
	terminate_all_threads();
	exit(EXIT_FAILURE);
  }
}

int uthread_spawn(thread_entry_point entry_point) {
    block_signals();
    if (!entry_point) {
        unblock_signals();
        return fail("Entry point can't be null");
    }
  int id = get_next_available_id();
  if (id >= MAX_THREAD_NUM) {
      unblock_signals();
      return fail("The number of threads reached maximum"); }
  thread* new_thread = new thread(id, entry_point);;
  threads[id] = new_thread;
  ready.push_back(new_thread);
  unblock_signals();
  return id;
}


int uthread_terminate(int tid) {
  block_signals();
  if (threads.find(tid) == threads.end()
	  || threads.find(tid)->second->get_state() == TERMINATED) {
	unblock_signals();
	return fail("Terminating this threads is forbidden");
  }
  if (!tid) {
      terminate_all_threads();
      exit(EXIT_SUCCESS);
  }
  if (running_id == tid) {
//	delete threads[tid];
    threads[tid]->free();
	threads[tid]->set_state(TERMINATED);
	timer_handler(SIGVTALRM);
  } else {
    threads[tid]->free();
	threads[tid]->set_state(TERMINATED);
	remove_thread();
	unblock_signals();
	return SUCCESS;
  }
  unblock_signals();
}

void remove_thread() {
  std::deque<thread*> tmp;
  while (!ready.empty()) {
	thread* tmp_thread = ready.front();
	ready.pop_front();
	if (tmp_thread->get_state() == READY) {
	  tmp.push_back(tmp_thread);
	}

  }
  ready = tmp;
}

int uthread_block(int tid) {
  block_signals();

  auto t = threads;

  if (threads.count(tid) == 0 || !tid) {
	unblock_signals();
	return fail("Blocking this thread is forbidden");
  }

  if (running_id == tid) {
	threads[tid]->set_state(BLOCKED);
	remove_thread();
	timer_handler(SIGVTALRM);
	unblock_signals();
	return SUCCESS;
  }

  if (threads[tid]->get_state() == READY) {
	  threads[tid]->set_state(BLOCKED);
	  remove_thread();
  }
  unblock_signals();
  return SUCCESS;
}

int uthread_resume(int tid){
  block_signals();

  if (threads.find(tid) == threads.end()){
	unblock_signals();
	return fail("This thread doesn't exist");
  }

  if (threads[tid]->get_state() == BLOCKED){
	threads[tid]->set_state(READY);
	ready.push_back(threads[tid]);
  }
  unblock_signals();
  return SUCCESS;
}

int uthread_sleep(int num_quantums) {
    block_signals();
    if (!running_id) {
        unblock_signals();
        return fail("Main thread can't sleep");
    }

    threads[running_id]->set_state(SLEEPING);
    threads[running_id]->set_sleep_time(num_quantums+1);
    timer_handler(SIGVTALRM);
    unblock_signals();
    return SUCCESS;
}


int uthread_get_tid() { return running_id; }

int uthread_get_total_quantums() { return total_quantums; }

int uthread_get_quantums(int tid) {
  block_signals();
  if (threads.find(tid) == threads.end()) {
	return fail("This thread doesn't exist");
  }
  unblock_signals();
  return threads[tid]->get_num_of_quantums();
}

int get_next_available_id() {
  int active_threads = 0;
  auto t = threads;
  for (auto const &it : threads) {
	if (it.second->get_state() != TERMINATED) {
	  active_threads++;
	} else { break; }
  }

  return active_threads;
}

void block_signals() {
  sigemptyset(&set);
  sigaddset(&set, SIGALRM);
  sigprocmask(SIG_BLOCK, &set, nullptr);
}

void unblock_signals() {
  sigprocmask(SIG_UNBLOCK, &set, nullptr);
}

void sleep_wake() {
    for (auto &it: threads) {
        if (it.second->get_state() == SLEEPING) {
            it.second->decrease_sleep_time();
            if (!it.second->get_sleep_time()) {
                it.second->set_state(READY);
                ready.push_back(it.second);
            }
        }
    }
}

void f() {
  int i = 0;
  while (1) {
	i++;
	if (i % 10000 == 0) {
	  std::cout << running_id;
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
      std::cout << running_id;
    }

  }

}

