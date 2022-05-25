#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <qe.h>
namespace near_mem {

bool near_mem::Qe::do_cycle() {

  if (working()) {
    remaining_cycle--;
    if (remaining_cycle == 0) {
      PLOG_DEBUG << "finished task at cycle: " << current_cycle;
    }
    return true;
  } else {
    if (!task_receiver.empty()) {
      PLOG_DEBUG << fmt::format("receive task from pe at cycle: {}",
                                current_cycle);
      auto next_task = task_receiver.pop();
      auto query_id = next_task.query_id;
      // process next task or send to lower pe according to query id
      remaining_cycle = 100;
      //
      return true;
    }
  }
  return false;
}
} // namespace near_mem
