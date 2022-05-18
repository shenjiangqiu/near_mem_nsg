#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H
#include <memory>
#include <queue>
#include <tuple>
#include<cassert>
namespace near_mem {

template <typename DataType> class TaskQueue {
public:
  unsigned size() const { return queue.size(); }
  DataType pop() {
    auto ret = queue.front();
    queue.pop();
    return ret;
  }
  DataType get() const { return queue.front(); }
  bool empty() const { return queue.empty(); }
  void push(DataType data) { queue.push(data); }
  void clear() { queue.clear(); }

private:
  std::queue<DataType> queue;
};

template <typename DataType> class Sender {
public:
  Sender(std::shared_ptr<TaskQueue<DataType>> queue, unsigned capacity)
      : task_queue(queue), capacity(capacity) {}
  bool full() const { return task_queue->size() >= capacity; }
  void push(DataType data) {
    assert(!full());
    task_queue->push(data);
  }
  unsigned size() const { return task_queue->size(); }

private:
  std::shared_ptr<TaskQueue<DataType>> task_queue;

  unsigned capacity;
};

template <typename DataType> class Receiver {
public:
  Receiver(std::shared_ptr<TaskQueue<DataType>> queue) : task_queue(queue) {}
  DataType get() { return task_queue->get(); }
  bool empty() { return task_queue->empty(); }
  DataType pop() { return task_queue->pop(); }
  unsigned size() const { return task_queue->size(); }

private:
  std::shared_ptr<TaskQueue<DataType>> task_queue;
};


// function to make a sender and a receiver
template <typename DataType>
std::tuple<Sender<DataType>, Receiver<DataType>>
make_task_queue(unsigned capacity) {
  auto task_queue = std::make_shared<TaskQueue<DataType>>();
  auto sender = Sender<DataType>(task_queue, capacity);
  auto receiver = Receiver<DataType>(task_queue);
  return std::make_tuple(sender, receiver);
}

} // namespace near_mem

#endif // TASK_QUEUE_H