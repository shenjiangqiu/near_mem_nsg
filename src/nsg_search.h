#ifndef NSG_SEARCH_H
#define NSG_SEARCH_H

#include <component.h>
#include <string>
#include <task_queue.h>
#include <struct.h>
#include <PrimitiveBloomFilter.h>

namespace near_mem {

// class nsg_search : public Component {
//     uint64_t state;
//     uint64_t current_cycle;
//     uint64_t query_id;
//     std::vector<std::vector<float>> query_data;
//     using TaskSender = Sender<PeTask>;
//     using TaskReceiver = Receiver<RetTask>;
// public:
//     nsg_search(uint64_t &current_cycle, TaskSender &&task_sender, TaskReceiver &&task_receiver)
//         : Component(current_cycle), task_sender(task_sender),
//             task_receiver(task_receiver) {}
//     void new_nsg_search(
//         std::vector<unsigned>& init_ids,
//         PrimitiveBloomFilter<unsigned,80000>& BF,
//         const float *query,
//         const float *base,
//         size_t K,
//         size_t L,
//         unsigned *indices,
//         bool thread_zero);
//     int get_data(int node_id, int* neighbor_node_list);
//     bool do_cycle() override;

//     nsg_search();
//     // {
//     //     state = query_init;
//     //     current_cycle = 0;
//     //     query_id = node_id;
//     //     query_dimension = dimension;
//     // }
    
// };

}

#endif // NSG_SEARCH_H