#include <controller.h>
#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
namespace near_mem {

bool near_mem::Controller::do_cycle() {
  bool busy = false;
  if (have_next_task()){
      if (!task_sender.full()) {
        auto next_query = current_query_id;
        // process this query
        auto query_node = get_query_node(next_query);
        task_sender.push(NsgTask{query_node});
        PLOG_DEBUG << fmt::format("send task {} to Query_Engine at cycle: {}", query_node, current_cycle);
        // if there is no more, break and set to end
        current_query_id++;
      }
  }

  return busy;
}

unsigned near_mem::Controller::get_query_node(unsigned query_id) {
  //TODO: get the query vector from the query id
  return query_id;
}


} // namespace near_mem