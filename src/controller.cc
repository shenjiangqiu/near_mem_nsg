#include <controller.h>
namespace near_mem {

bool near_mem::Controller::do_cycle() {
  bool busy = false;
  if (have_next_task() and !task_sender.full()) {
    task_sender.push(generate_next_task());
    on_going_reqs++;
    busy = true;
  }
  if (!task_receiver.empty()) {
    on_going_reqs--;
    task_receiver.pop();
    busy = true;
  }
  return busy;
}
} // namespace near_mem