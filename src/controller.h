#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <component.h>
#include <efanna2e/distance.h>
#include <efanna2e/exceptions.h>
#include <efanna2e/index_nsg.h>
#include <efanna2e/parameters.h>
#include <efanna2e/util.h>
#include <string>
#include <struct.h>
#include <task_queue.h>
#include <vector>
namespace near_mem {
using namespace efanna2e;

// the controller class take the input from NSG and generate task to PEs
//
class Controller : Component {
  using TaskSender = Sender<PeTask>;
  using TaskReceiver = Receiver<RetTask>;

public:
  Controller(NsgTask &&task, uint64_t &current_cycle,
             std::vector<TaskSender> &&task_sender,
             std::vector<TaskReceiver> &&task_receiver, const size_t dimension,
             const size_t n, Metric m, Index *initializer,
             const float *query_load, unsigned query_num,
             unsigned query_dimenstion)
      : Component(current_cycle), index(dimension, n, m, initializer),
        task(task), task_sender(task_sender), task_receiver(task_receiver),
        query_load(query_load), query_num(query_num),
        query_dimenstion(query_dimenstion) {}

  bool do_cycle() override;
  std::string get_internal_size() const override {
    return std::string("");
  }; // TODO add the internal status for the controller for debug
  std::string get_line_trace() const override { return std::string(""); }; //

private:
  // input parameters and data
  // TODO implement it to test if there is next
  // task
  bool have_next_task() const { return current_query_id < query_num; }

  efanna2e::IndexNSG index;

  // TODO implement it to generate the next task
  PeTask generate_next_task();
  unsigned on_going_reqs = 0;
  NsgTask task;
  std::vector<TaskSender> task_sender;
  std::vector<TaskReceiver> task_receiver;

  // this data is mannaged by outside, don't delete it
  const float *query_load;
  unsigned query_num;
  unsigned query_dimenstion;

  // runtime data
  unsigned current_query_id = 0;
};

} // namespace near_mem

#endif // CONTROLLER_H