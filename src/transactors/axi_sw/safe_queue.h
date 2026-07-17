// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// https://stackoverflow.com/a/16075550
#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>

// A threadsafe-queue.
template <class T>
class SafeQueue {
public:
  SafeQueue(void)
      : q(), m(), c() {}

  ~SafeQueue(void) {}

  // Add an element to the queue.
  void enqueue(const T& t) {
    std::lock_guard<std::mutex> lock(m);
    q.push(t);
    c.notify_one();
  }

  void enqueue(T&& t) {
    std::lock_guard<std::mutex> lock(m);
    q.push(std::move(t));
    c.notify_one();
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is available.
  T dequeue(void) {
    std::unique_lock<std::mutex> lock(m);
    while (q.empty()) {
      // release lock as long as the wait and reaquire it afterwards.
      c.wait(lock);
    }
    T val = std::move(q.front());
    q.pop();
    return val;
  }

  // Pop the "front"-element if not empty
  std::pair<bool, T> try_dequeue(void) {
    std::unique_lock<std::mutex> lock(m);
    if (q.empty()) {
      return std::make_pair(false, T());
    }
    T val = std::move(q.front());
    q.pop();
    return std::make_pair(std::move(true), std::move(val));
  }

  // Get the "front"-element if not empty
  std::pair<bool, T> try_peek(void) {
    std::unique_lock<std::mutex> lock(m);
    if (q.empty()) {
      return std::make_pair(false, T());
    }
    return std::make_pair(true, q.front());
  }

  // Clear all elements.
  void clear(void) {
    std::lock_guard<std::mutex> lock(m);
    std::queue<T> empty;
    q.swap(empty);
  }

  bool empty(void) {
    std::unique_lock<std::mutex> lock(m);
    return q.empty();
  }

  size_t size(void) {
    std::unique_lock<std::mutex> lock(m);
    return q.size();
  }

  T& front(void) {
    std::unique_lock<std::mutex> lock(m);
    return q.front();
  }

private:
  std::queue<T> q;
  mutable std::mutex m;
  std::condition_variable c;
};
#endif
