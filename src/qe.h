#ifndef QE_H
#define QE_H

#include <component.h>
#include <struct.h>
#include <task_queue.h>
#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>

namespace near_mem {
class Query_Engine : public Component {
  using TaskReceiver = Receiver<NsgTask>;
  using DistanceSender = Sender<DistanceTask>;
  using DistanceReceiver = Receiver<DistanceTask>;
  using MemSender = Sender<uint64_t>;
  using MemReceiver = Receiver<uint64_t>;

public:
  Query_Engine(
    uint64_t &current_cycle, 
    uint64_t qe_name,
    TaskReceiver task_receiver, 
    DistanceSender distance_sender,
    DistanceReceiver return_distance_receiver,
    MemSender mem_sender,
    MemReceiver mem_receiver
    ) : 
  Component(current_cycle), 
  qe_name(qe_name),
  task_receiver(task_receiver),
  distance_sender(distance_sender),
  return_distance_receiver(return_distance_receiver),
  mem_sender(mem_sender),
  mem_receiver(mem_receiver) {}

  bool do_cycle() override;
  bool all_end() const override {
    auto end = !working();
    return end;
  }
  std::string get_internal_size() const override {
    return std::string("");
  }; // TODO add the internal status for the controller for debug
  std::string get_line_trace() const override { 
    return std::string(""); 
  }; //
  bool working() const { 
    return remaining_cycle > 0; 
  }

private:
  uint64_t qe_name;
  TaskReceiver task_receiver;
  DistanceSender distance_sender;
  DistanceReceiver return_distance_receiver;
  MemSender mem_sender;
  MemReceiver mem_receiver;
  // TODO: Query state machine
  bool palse = false;
  unsigned palse_id = 0;//TODO: vector<unsigned> palse_list;
  unsigned query_id = 0;
  unsigned remaining_cycle = 0;
};
} // namespace near_mem

#endif