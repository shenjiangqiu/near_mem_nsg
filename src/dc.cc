#include <dc.h>

namespace near_mem {

bool near_mem::Distance_Computer::do_cycle() {
    if (working()) {
        unsigned counter_minus = 0;
        unsigned counter_multi = 0;
        bool all_computation_finished = true;
        for (auto &sub_unit : dc_task_state) {
            // std::cout << "sub_unit.state: " << sub_unit.state << " sub_unit.remaining_cycle: " << sub_unit.remaining_cycle << std::endl;
            switch (sub_unit.state) {
                case MINUS_PENDING:
                    all_computation_finished = false;
                    counter_minus++;
                    if (counter_minus > num_compute_unit) {
                        break;
                    }
                    sub_unit.remaining_cycle--;
                    if (sub_unit.remaining_cycle == 0) {
                        sub_unit.remaining_cycle = latency_compute_unit;
                        sub_unit.state = MULTI_PENDING;
                    }
                    break;
                case MULTI_PENDING:
                    all_computation_finished = false;
                    counter_multi++;
                    if (counter_multi > num_compute_unit) {
                        break;
                    }
                    sub_unit.remaining_cycle--;
                    if (sub_unit.remaining_cycle == 0) {
                        // sub_unit.remaining_cycle = latency_compute_unit; //TODO: final plus latency
                        sub_unit.state = PLUS_PENDING; //FINISH - & *
                    }
                    break;
                default:
                    break;
            }
        }
        if (all_computation_finished) {
            if (!return_distance_sender.full()){
                PLOG_DEBUG << "DC name " << dc_name << " finished task at cycle: " << current_cycle << " of node_id: " << node_id << " of qe_name: " << occupy_qe_name;
                return_distance_sender.push(DistanceTask{occupy_qe_name, query_id, node_id});
                dc_task_state.clear();
                dc_work_finished = true;
                return true;
            } else {
                PLOG_DEBUG << "DC name " << dc_name << " return_distance_sender is full at cycle: " << current_cycle  << " of node_id: " << node_id ;
                return false;
            }
        }
        return true;
    } else {
        if (!distance_receiver.empty()) {
            auto next_task = distance_receiver.pop();
            occupy_qe_name = next_task.qe_name;
            query_id = next_task.query_id;
            node_id = next_task.node_id;
            dc_work_finished = false;
            for (auto &sub_unit : dc_task_state) {
                sub_unit.remaining_cycle = latency_compute_unit;
                sub_unit.state = MINUS_PENDING;
                // std::cout << "11sub_unit.state: " << sub_unit.state << " sub_unit.remaining_cycle: " << sub_unit.remaining_cycle << std::endl;
            }
            PLOG_DEBUG << fmt::format("DC name {} receive task {} from QE {} at cycle: {}",
                                      dc_name, node_id, occupy_qe_name, current_cycle);
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
