#include <controller.h>
#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
namespace near_mem {

bool near_mem::Controller::do_cycle() {
  bool busy = false;
  if (have_next_task())
    for (auto &sender : task_sender) {
      if (!sender.full()) {
        auto next_query = current_query_id;
        // process this query
        sender.push(NsgTask{10});
        PLOG_DEBUG << fmt::format("send task to pe at cycle: {}",
                                  current_cycle);
        // if there is no more, break and set to end
        current_query_id++;
        if (current_query_id >= query_num) {
          current_query_id = query_num;
          break;
        }
      }
    }

  return busy;
}
} // namespace near_mem