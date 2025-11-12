#include "pool.h"
#include <mutex>
#include <iostream>

Task::Task() = default;
Task::~Task() = default;

ThreadPool::ThreadPool(int num_threads) {
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back(new std::thread(&ThreadPool::run_thread, this));
  }
}

ThreadPool::~ThreadPool() {
  for (std::thread *t: threads) {
    delete t;
  }
  threads.clear();
  
  for (Task *q: queue) {
    delete q;
  }
  queue.clear();
}

void ThreadPool::SubmitTask(const std::string &name, Task *task) {
  //TODO: Add task to queue, make sure to lock the queue
  std::lock_guard<std::mutex> lock(mtx);
  if (done) {
    std::cout << "Cannot add task " << name << ": pool is stopping." << std::endl;
    //delete task;
    return;
  }
  task->name = name;
  task->running = false;
  queue.push_back(task);
  num_tasks_unserviced++;
  std::cout << "Added task " << name << std::endl;
}

void ThreadPool::run_thread() {
  while (true) {
    Task *t = nullptr;
    {

      std::lock_guard<std::mutex> lock(mtx);

      //TODO1: if done and no tasks left, break
      if (!queue.empty()) {
	t = queue.front();
	queue.erase(queue.begin());
	num_tasks_unserviced--;
	t->running = true;
      }
      else if (done) {
	break; // exit if no tasks and pool is stopping
      }
    }
      
    if (t) {
      std::cout << "Started task" << std::endl;
      try{t->Run();} catch(...) {}
      t->running = false;
      std::cout << "Finished task" << std::endl;
      delete t; //TODO4: delete task 
    }
    else {
      if (done) break;
      std::this_thread::yield(); // wait briefly
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  std::cout << "Stopping thread" << std::endl;
}

// Remove Task t from queue if it's there
void ThreadPool::remove_task(Task *t) {
  mtx.lock();
  for (auto it = queue.begin(); it != queue.end();) {
    if (*it == t) {
      queue.erase(it);
      mtx.unlock();
      return;
    }
    ++it;
  }
  mtx.unlock();
}

void ThreadPool::Stop() {
  //TODO: Delete threads, but remember to wait for them to finish first
  {
    std::lock_guard<std::mutex> lock(mtx);
    done = true;
  }
  std::cout << "Stopping thread pool..." << std::endl;
  
  for (auto *th : threads) {
    if (th->joinable()) th->join();
    delete th;
  }
  threads.clear();

  // clean up per-task condition variables
  {
    std::lock_guard<std::mutex> lock(mtx);
    for (Task* t : queue) delete t;
    queue.clear();
  }
  std::cout << "All threads stopped." << std::endl;
}
