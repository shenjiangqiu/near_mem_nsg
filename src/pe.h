#ifndef PE_H
#define PE_H

#include <component.h>
#include <string>
#include <task_queue.h>
#include <struct.h>
#include <nsg_search.h>

namespace near_mem {


template <typename TaskType> 
class PE : public Component {
  int corrent_node;
  int neighbor_node_list[256];
  void init_query(int node_id){
    corrent_node = node_id;
    for(int i = 0; i < 256; i++){
      neighbor_node_list[i] = -1;
    }
    // auto nsg_search = new nsg_search(corrent_node);
  }
  public:

    // public:
    // PE(uint64_t &current_cycle, TaskSender &&task_sender, TaskReceiver &&task_receiver)
    //     : Component(current_cycle), task_sender(task_sender),
    //         task_receiver(task_receiver) {}
    
    // bool do_cycle() override;
    // std::string get_internal_size() const override {
    //     return std::string("");
    // }; // TODO add the internal status for the controller for debug
    // std::string get_line_trace() const override { return std::string(""); }; //
    
    // private:
    // TaskReceiver task_receiver;
    // TaskSender task_sender;
};


} // namespace near_mem

#endif // PE_H