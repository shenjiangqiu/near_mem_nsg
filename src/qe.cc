#include <qe.h>

namespace near_mem {

bool near_mem::Query_Engine::do_cycle() {

  if (working()) {
    if (palse == true){
      if (!return_distance_receiver.empty()){
        auto qe_top_name = return_distance_receiver.get().qe_name;
        auto qe_top_id = return_distance_receiver.get().query_id;
        if (qe_top_name == qe_name && qe_top_id == palse_id){ //TODO: check a list of palse_id
          return_distance_receiver.pop();
          palse = false;
          remaining_cycle--;
          PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is done", qe_name, palse_id);
          return true;
        }
      }
      PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is still palsing", qe_name, palse_id);
      return false;
    }
    if (remaining_cycle == 5){ //send DC task to DC, and set state palse, Can continue sending DC task to DC
      if(!distance_sender.full()){
        distance_sender.push(DistanceTask{qe_name, query_id});
        PLOG_DEBUG << fmt::format("Palse task {} to Distance_Computer at cycle: {}", query_id, current_cycle);
        palse = true;
        palse_id = query_id;
        return true;
      }
      return false;
    }
    remaining_cycle--;

    PLOG_DEBUG << "QE name " << qe_name << " remaining_cycle: " << remaining_cycle << " current_cycle: " << current_cycle;
    if (remaining_cycle == 0) {
      PLOG_DEBUG << "QE name " << qe_name << " finished task at cycle: " << current_cycle << " current_cycle: " << current_cycle;
    }
    return true;
  } else {
    if (!task_receiver.empty()) {
      auto next_task = task_receiver.pop();
      query_id = next_task.query_id;
      // process next task or send to lower pe according to query id
      remaining_cycle = 10;
      PLOG_DEBUG << fmt::format("QE name {} receive task {} from Controller at cycle: {}, remain cycle={}",
                                qe_name, query_id, current_cycle, remaining_cycle);
      //
      return true;
    }else{
      PLOG_DEBUG << fmt::format("QE name {} NO TASK at cycle: {}",
                                qe_name, current_cycle);
      return false;
    }
  }
  return false;
}
} // namespace near_mem
