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
  std::unique_lock<std::mutex> lock(queue_mutex);
  if (done) {
    // If Stop() was already called, do not accept new tasks
    delete task;
    return;
  }
  queue.push_back(task);  // add new task
  cv.notify_one();  // wake up one waiting worker thread
}

void ThreadPool::run_thread() {
  while (true) {
    Task *t = nullptr;
    std::unique_lock<std::mutex> lock(queue_mutex);
    cv.wait(lock, [&] { return done || !queue.empty(); });
    //TODO1: if done and no tasks left, break
    if (done && queue.empty())
      break;
    //TODO2: if no tasks left, continue
    t = queue.front();
    //TODO3: get task from queue, remove it from queue, and run it
    queue.pop_front();
    //run task outside lock
    if (t) {
      t->Run();  // run user code
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
  std::unique_lock<std::mutex> lock(queue_mutex);
  done = true;
  cv.notify_all();  // wake up all workers so they can exit
  
  for (auto *t : threads) {
    if (t->joinable())
      t->join();
    delete t;
  }
  threads.clear();
}
