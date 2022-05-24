#ifndef QE_H
#define QE_H

#include <component.h>
#include <struct.h>
#include <task_queue.h>
#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
namespace near_mem {
class Qe : public Component {
  using TaskSender = Sender<QeTask>;
  using TaskReceiver = Receiver<NsgTask>;
  using DistanceReceiver = Receiver<DistanceTask>;

public:
  Qe(uint64_t &current_cycle, std::vector<TaskSender> task_sender,
     TaskReceiver task_receiver)
      : Component(current_cycle), task_sender(std::move(task_sender)),
        task_receiver(task_receiver) {}

  bool do_cycle() override;
  bool all_end() const override {
    auto end = !working();
    return end;
  }
  std::string get_internal_size() const override {
    return std::string("");
  }; // TODO add the internal status for the controller for debug
  std::string get_line_trace() const override { return std::string(""); }; //
  bool working() const { return remaining_cycle > 0; }

private:
  std::vector<TaskSender> task_sender;

  TaskReceiver task_receiver;

  std::vector<DistanceReceiver> distance_receiver;

  unsigned remaining_cycle = 0;
};
} // namespace near_mem

#endif