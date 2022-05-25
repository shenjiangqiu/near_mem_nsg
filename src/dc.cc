#include <dc.h>

namespace near_mem {

bool near_mem::Distance_Computer::do_cycle() {
    if (working()) {
        remaining_cycle--;
        PLOG_DEBUG << "DC name " << dc_name << " remaining_cycle: " << remaining_cycle << " current_cycle: " << current_cycle;
        if (remaining_cycle == 0) {
            if (!return_distance_sender.full()){
                PLOG_DEBUG << "DC name " << dc_name << " finished task at cycle: " << current_cycle << " of query_id: " << query_id << " of qe_name: " << occupy_qe_name;
                return_distance_sender.push(DistanceTask{occupy_qe_name, query_id});
            } else {
                remaining_cycle++;
                return false;
                PLOG_DEBUG << "DC name " << dc_name << " return_distance_sender is full at cycle: " << current_cycle << " remaining_cycle: " << remaining_cycle;
            }
        }
        return true;
    } else {
        if (!distance_receiver.empty()) {
            auto next_task = distance_receiver.pop();
            query_id = next_task.query_id;
            occupy_qe_name = next_task.qe_name;
            remaining_cycle = 2;
            PLOG_DEBUG << fmt::format("DC name {} receive task {} from QE {} at cycle: {}, remain cycle={}",
                                      dc_name, query_id, occupy_qe_name, current_cycle, remaining_cycle);
            return true;
        } else {
            PLOG_DEBUG << fmt::format("DC name {} NO TASK at cycle: {}",
                                      dc_name, current_cycle);
            return false;
        }
    }

    return false;
}
} // namespace near_mem
