#include <qe.h>

using namespace std;

namespace near_mem {

bool near_mem::Query_Engine::do_cycle() {
  PLOG_DEBUG << "working? " << working() << " state: " << +query_state;
  if (working()) {
    switch(query_state){
      case query_init: //200 init_ids dc request: no need to send mem request
        {
          //generate & send init dc request
          //init_dc_state: 0: not send, 1: send but not receive, 2: send and receive
          bool dc_ready_all = true;
          for (unsigned i = 0; i < L; ++i){
            if(init_dc_state[i] != FINISHED){
              dc_ready_all = false;
              if(!distance_sender.full() && init_dc_state[i] == NOT_SEND){
                unsigned dc_node_id = query_id*1000+i;
                distance_sender.push(DistanceTask{qe_name, dc_node_id});
                PLOG_DEBUG << fmt::format("Palse task {} to Distance_Computer at cycle: {}", query_id, current_cycle);
                init_dc_state[i] = INFLIGHT;
                return 1;
              }
              if(init_dc_state[i] == INFLIGHT){
                if(!return_distance_receiver.empty()){
                  auto qe_top_name = return_distance_receiver.get().qe_name;
                  auto qe_top_id = return_distance_receiver.get().query_id;
                  if (qe_top_name == qe_name && qe_top_id == query_id*1000+i){ //TODO: check a list of palse_id
                    return_distance_receiver.pop();
                    //write into retset
                    init_dc_state[i] = FINISHED;
                    PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is done", qe_name, i);
                    return 1;
                  }
                }
              }
              return 0;
            }
          }
          if (dc_ready_all == true){
            query_state = query_init_finish;
            //TODO: index.NewSearchInit(init_ids, retset, query_load + query_id * dim, data_load, L);
            //init for following while loop
            return 1;
          }
        }
        break;

      case query_init_finish:
        //sort retset
        {
          while_k = 0;
          query_state = query_get_data;
          // query_state = query_finish;
          //TODO: Read Edge Table based on while_k
          dc_task_list.clear();
          PLOG_DEBUG << "sort retset, init done";
        }
        break;
      case query_get_data://query_get_data
        {
          std::vector<unsigned> target_ids;
          while_k = index.NewSearchGetData(retset, flags, query_load + query_id * dim, data_load, L, while_k, target_ids);
          if (while_k == -1){
            query_state = query_finish;
            return 1;
          }
          else if (target_ids.empty()){
            return 1;
            //for this k there is no DC tasks
          }else{
            for(auto id : target_ids){
              dc_task_list.push_back(QE_DC_Task{qe_name, query_id, id, qe_name*1000+id, NOT_SEND});
            }
            //TODO: generate edge read request
            query_state = query_pending_edge;
            return 1;
          }
            
        }
        break;
      case query_pending_edge:
        {
          bool all_edge_read_done = true;
          //TODO: pending edge read
          if(all_edge_read_done){
            query_state = query_pending_distance;
            return 1;
          }else{
            return 0;
          }
        }
        break;
      case query_pending_distance://pending for dc_task_list
        {

          for (auto dc_task : dc_task_list){
            //TODO state: NOT_READ, READ_INFLIGHT, READ_FINISHED, DC_INFLIGHT, DC_FINISHED
            if (!mem_sender.full() && dc_task.state == READ_NOT_SEND){
              mem_sender.push(MemTask{dc_task.mem_read_addr, dc_task.true, qe_name, dc_task.id, false});
              dc_task.state = READ_INFLIGHT;
              PLOG_DEBUG << fmt::format("QE name {}: send mem request to Mem_Computer at cycle: {}", qe_name, current_cycle);
              return 1;
            }
            if (!mem_receiver.empty() && dc_task.state == READ_INFLIGHT){
              auto mem_top_name = mem_receiver.get().qe_name;
              auto mem_top_id = mem_receiver.get().node_id;
              if (mem_top_name == qe_name && mem_top_id == dc_task.node_id){
                mem_receiver.pop();
                dc_task.state = READ_FINISHED;
                return 1;
              }
            }
            if(!distance_sender.full() && dc_task.state == READ_FINISHED){//read finish judge is a vector
              //send dc task
              distance_sender.push(DistanceTask{dc_task.qe_name, dc_task.node_id});
              PLOG_DEBUG << fmt::format("Palse task {} to Distance_Computer at cycle: {}", dc_task.query_id, current_cycle);
              dc_task.state = DC_INFLIGHT;
              return 1;
            }
            if(!return_distance_receiver.empty() && dc_task.state == DC_INFLIGHT){
              auto qe_top_name = return_distance_receiver.get().qe_name;
              auto qe_top_id = return_distance_receiver.get().query_id;
              if (qe_top_name == qe_name && qe_top_id == dc_task.node_id){ //TODO: check a list of palse_id
                return_distance_receiver.pop();
                dc_task.state = DC_FINISHED;
                //write into retset
                PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is done", qe_name, dc_task.query_id);
                return 1;
              }
            }
          }
          if(1/*all vector read+DC is done*/){
            query_state = query_get_data;
          }
        }
        break;
      case query_finish:
        // for (size_t i = 0; i < K; i++) {
        //   indices[i] = retset[i].id;
        // }
        // fixed 
        PLOG_DEBUG << "query_finish";
        break;
      default:
        assert(0);
        break;
    }

    // remaining_cycle--;

    PLOG_DEBUG << "QE name " << qe_name << " state: " << +query_state << " current_cycle: " << current_cycle;
    if (remaining_cycle == 0) {
      PLOG_DEBUG << "QE name " << qe_name << " finished task at cycle: " << current_cycle << " current_cycle: " << current_cycle;
    }
    return 1;
  } else { //QE is not working, fetch new task
    if (!task_receiver.empty()) {
      auto next_task = task_receiver.pop();
      query_id = next_task.query_id;
      // process next task or send to lower pe according to query id
      // remaining_cycle = 10;
      query_state = query_init;
      //init the return_set, flags, and init_id to the inited global value.
      retset = global_retset;
      init_ids = global_init_ids;
      flags = global_flags;
      for (unsigned i = 0; i < L; i++) {
        query_init_state[i] = READ_FINISHED; // READ_NOT_SEND;
        init_mem_state[i] = FINISHED;
        init_dc_state[i] = NOT_SEND;
      }
      PLOG_DEBUG << fmt::format("QE name {} receive task {} from Controller at cycle: {}, query_state={}",
                                qe_name, query_id, current_cycle, +query_state);
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


int near_mem::Query_Engine::dc_test(){
  if (palse == true){
    if (!return_distance_receiver.empty()){
      auto qe_top_name = return_distance_receiver.get().qe_name;
      auto qe_top_id = return_distance_receiver.get().query_id;
      if (qe_top_name == qe_name && qe_top_id == palse_id){ //TODO: check a list of palse_id
        return_distance_receiver.pop();
        palse = false;
        remaining_cycle--;
        PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is done", qe_name, palse_id);
        return 1;
      }
    }
    PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is still palsing", qe_name, palse_id);
    return 0;
  }
  if (remaining_cycle == 5){ //send DC task to DC, and set state palse, Can continue sending DC task to DC
    if(!distance_sender.full()){
      distance_sender.push(DistanceTask{qe_name, query_id});
      PLOG_DEBUG << fmt::format("Palse task {} to Distance_Computer at cycle: {}", query_id, current_cycle);
      palse = true;
      palse_id = query_id;
      return 1;
    }
    return 0;
  }
  return 2;
}

void near_mem::Query_Engine::print_Neighbor(const efanna2e::Neighbor nn){
  cout << "Neighbor: " << nn.id << " " << nn.distance << " " << nn.flag << " ";
}

int near_mem::Query_Engine::get_data(int k){
  return k;
}


} // namespace near_mem
