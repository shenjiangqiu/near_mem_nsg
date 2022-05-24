#include <controller.h>
namespace near_mem {

bool near_mem::Controller::do_cycle() {
  bool busy = false;
  if (have_next_task())
    for (auto &sender : task_sender) {
      auto next_query = current_query_id;
      // process this query
      
      
      // if there is no more, break and set to end
      current_query_id++;
      if (current_query_id >= query_num) {
        current_query_id = query_num;
        break;
      }
    }


  return busy;
}
} // namespace near_mem