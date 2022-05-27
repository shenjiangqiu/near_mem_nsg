#include <qe.h>

using namespace std;

namespace near_mem {

bool near_mem::Query_Engine::do_cycle() {
  PLOG_DEBUG << "working? " << working() << " state: " << +query_state;
  if (working()) {
    //test case: simple palse
    // int dc_ret = dc_test();
    // if (dc_ret != 2){
    //   return (bool)dc_ret;
    // }
    //test case: simple palse end
    switch(query_state){
      case query_init: //read_vec_of_init_id
        {
          bool mem_ready_all = true;
          for (unsigned i = 0; i < 200; ++i) {
            if(init_mem_state[i] != FINISHED){
              PLOG_DEBUG << "xxx " << i;
              mem_ready_all = false;
              //generate & send mem read request, use mem_read queue
              //memsend_state: 0: not send, 1: send but not receive, 2: send and receive
              return 0;
            }
          }
          if (mem_ready_all == true){
            query_state = query_init_pending;
            return 1;
          }
        }
        break;
      case query_init_pending:
        //TODO: mem_receiver, create a vector of return mem address, let each state machine to find its callback
        {
          bool dc_ready_all = true;
          for (unsigned i = 0; i < 200; ++i){
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
              //generate & send init dc request
              //init_dc_state: 0: not send, 1: send but not receive, 2: send and receive
              return 0;
            }
          }
          if (dc_ready_all == true){
            query_state = query_init_finish;
            //init for following while loop
            while_k = 0;
            while_nk = 0;
            for (unsigned i = 0; i < 200; ++i) {
              for (unsigned j = 0; j < 50; ++j) {
                while_edge_table_read_state[i][j] = 0;
                while_vec_read_state[i][j] = 0;
              }
            }
            return 1;
          }
        }
        break;
      case query_init_finish:
        //sort retset
        {
          query_state = query_while_loop;
          PLOG_DEBUG << "sort retset, init done";
        }
        break;
      case query_while_loop://query_loop_edge_read
        {
          if (while_k >= 200){
            query_state = query_done;
            return 1;
          }
          if (while_enter_flag[while_k] == true){
            bool edge_read_finished = true;
            for(unsigned i = 0; i < 200; ++i){
              if(while_edge_table_read_state[while_k][i] != FINISHED){
                edge_read_finished = false;
                if(!distance_sender.full() && while_edge_table_read_state[while_k][i] == NOT_SEND){
                  //send to mem (multi)
                  while_n = retset[while_k].id;
                  mem_sender.push(MemReadTask{qe_name, while_n, true});
                  while_edge_table_read_state[while_k][i] = INFLIGHT;
                  return 1;
                }
                if(while_edge_table_read_state[while_k][i] == INFLIGHT){
                  if(!mem_receiver.empty()){
                    auto qe_top_name = mem_receiver.get().qe_name;
                    auto qe_top_id = mem_receiver.get().node_id;
                    auto qe_top_is_edge = mem_receiver.get().is_edge_table;
                    if (qe_top_name == qe_name && qe_top_id == while_n && qe_top_is_edge == true){ //TODO: check a list of palse_id
                      //how to receive multi
                      mem_receiver.pop();
                      //write into retset
                      while_edge_table_read_state[while_k][i] = FINISHED;
                      return 1;
                    }
                  }
                }
                return 0;
              }
            }
            if (edge_read_finished == true){
              query_state = query_end_judge;
              while_m = 0;
              return 1;
            }
          } else { // don't enter the flag
            query_state = query_end_pending;
          }
        }
        break;
      case query_end_judge: // query_loop_vec_read
        //TODO: how to deal with the 2 continues?
        {
          if(while_m >= 50){
            query_state = query_end_pending;
            return 1;
          }
          //read_vector_from_memory
          unsigned id = final_graph_[while_n][while_m];
          if (flag[id]){// or BF
            while_m++;// "continue"
            return 1;
          }
          if (while_vec_read_state[while_k][while_m] != FINISHED){
            if(!mem_sender.full() && while_vec_read_state[while_k][while_m] == NOT_SEND){
              //send to mem (multi)
              
              return 1;
            }
            if (while_vec_read_state[while_k][while_m] == INFLIGHT){
              if(!mem_receiver.empty()){
                auto qe_top_name = mem_receiver.get().qe_name;
                auto qe_top_id = mem_receiver.get().node_id;
                auto qe_top_is_edge = mem_receiver.get().is_edge_table;
                if (qe_top_name == qe_name && qe_top_id == while_n && qe_top_is_edge == false){ //TODO: check a list of palse_id
                  //how to receive multi
                  mem_receiver.pop();
                  //write into retset
                  while_vec_read_state[while_k][while_m] = FINISHED;
                  return 1;
                }
              }
            }
            return 0;
          }
          //if here, all vec is read, start to DC
          
          if(init_dc_state[while_m] != FINISHED){
              if(!distance_sender.full() && init_dc_state[while_m] == NOT_SEND){
                unsigned dc_node_id = query_id*1000+while_m;
                distance_sender.push(DistanceTask{qe_name, dc_node_id});
                PLOG_DEBUG << fmt::format("Palse task {} to Distance_Computer at cycle: {}", query_id, current_cycle);
                init_dc_state[while_m] = INFLIGHT;
                return 1;
              }
              if(init_dc_state[while_m] == INFLIGHT){
                if(!return_distance_receiver.empty()){
                  auto qe_top_name = return_distance_receiver.get().qe_name;
                  auto qe_top_id = return_distance_receiver.get().query_id;
                  if (qe_top_name == qe_name && qe_top_id == query_id*1000+while_m){ //TODO: check a list of palse_id
                    return_distance_receiver.pop();
                    //write into retset
                    init_dc_state[while_m] = FINISHED;
                    PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is done", qe_name, i);
                    return 1;
                  }
                }
              }
              //generate & send init dc request
              //init_dc_state: 0: not send, 1: send but not receive, 2: send and receive
              return 0;
          }
          //if here, dc is done, start to insert
          float dist = xxx;
          if (dist >= retset[L - 1].distance){
            while_m++;
            return 1;
          }
          while_r = InsertIntoPool(retset.data(), L, nn);
          if (while_r < while_nk){
            while_nk = while_r;
            while_m++;
            return 1;
          }
        }
        break;
      case query_end_pending: //go to next while loop
        {
          if (while_nk <= while_k){
            while_k = while_nk;
          } else {
            while_k++;
          }
          query_state = query_while_loop;
          return 1;
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
      for (unsigned i = 0; i < 200; ++i) {
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
} // namespace near_mem
