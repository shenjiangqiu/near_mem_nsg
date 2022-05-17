#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <component.h>
#include <string>
#include <task_queue.h>
namespace near_mem {
struct NsgTask {

  /*
      the task that nsg send to controller,
      this should describe a single query of NSG
  */
};

struct PeTask {
  /*
      todo the pe task that send to PE
  */
};
struct RetTask {
  /*
      todo the ret task that send from PE
  */
};
// the controller class take the input from NSG and generate task to PEs
//
class Controller : Component {
  using TaskSender = Sender<PeTask>;
  using TaskReceiver = Receiver<RetTask>;

public:
  Controller(NsgTask &&task, uint64_t &current_cycle, TaskSender &&task_sender,
             TaskReceiver &&task_receiver)
      : task(task), Component(current_cycle), task_sender(task_sender),
        task_receiver(task_receiver) {}

  bool do_cycle() override;
  std::string get_internal_size() const override {
    return std::string("");
  }; // TODO add the internal status for the controller for debug
  std::string get_line_trace() const override { return std::string(""); }; //

private:
  // TODO implement it to test if there is next
  // task
  bool have_next_task() const;

  // TODO implement it to generate the next task
  PeTask generate_next_task();
  unsigned on_going_reqs = 0;
  NsgTask task;
  TaskSender task_sender;
  TaskReceiver task_receiver;
};

} // namespace near_mem

#endif // CONTROLLER_H