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
    // If Stop() was already called, do not accept new tasks
    delete task;
    return;
  }
  std::cout << "Added task " << name << std::endl;
  queue.push_back(task);  // add new task

  // prepare per-task done signaling
  task_done_map[task] = false;
  task_cv_map[task] = new std::condition_variable();
  
  cv.notify_one();  // wake up one worker thread
}

void ThreadPool::run_thread() {
  while (true) {
    Task *t = nullptr;
    std::lock_guard<std::mutex> lock(mtx);
    //TODO1: if done and no tasks left, break
    if (done && queue.empty()){
      std::cout << "Stopping thread" << std::endl;
      break;
    }
    if (!queue.empty()){
      //TODO2: if no tasks left, continue
      t = queue.front();
      //TODO3: get task from queue, remove it from queue, and run it
      queue.pop_front();
    }
    //run task outside lock
    if (t) {
      std::cout << "Started task" << std::endl;
      t->Run();  // run user code
      std::cout << "Finished task" << std::endl;

      // mark task as done and notify any waiting thread
      std::lock_guard<std::mutex> lock(mtx);
      task_done_map[t] = true;
      task_cv_map[t]->notify_all();
      
      delete t; //TODO4: delete task 
    }
  }
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
  std::lock_guard<std::mutex> lock(mtx);
  done = true;
  std::cout << "Stopping thread pool..." << std::endl;
  cv.notify_all();  // wake all threads so they can exit
  
  for (auto *th : threads) {
    if (th->joinable())
      th->join();
    delete th;
  }
  threads.clear();

  // clean up per-task condition variables
  for (auto &[t, cv_ptr] : task_cv_map){
    delete cv_ptr;}
  task_cv_map.clear();
  task_done_map.clear();
  
  std::cout << "All threads stopped." << std::endl;
}
