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
                unsigned dc_node_id = retset[i].id;
                distance_sender.push(DistanceTask{qe_name, query_id, dc_node_id});
                PLOG_DEBUG << fmt::format("Palse task {} to Distance_Computer at cycle: {}", query_id, current_cycle);
                init_dc_state[i] = INFLIGHT;
                return 1;
              }
              if(init_dc_state[i] == INFLIGHT){
                if(!return_distance_receiver.empty()){
                  auto qe_top_name = return_distance_receiver.get().qe_name;
                  auto qe_top_id = return_distance_receiver.get().query_id;
                  auto qe_top_node_id = return_distance_receiver.get().node_id;
                  if (qe_top_name == qe_name && qe_top_id == query_id && qe_top_node_id == retset[i].id){ 
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
            global_index->NewSearchInit(init_ids, retset, query_load + query_id * dim, data_load, L);
            PLOG_DEBUG << "NewSearchInit done";
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
          // TODO: sort
          dc_task_list.clear();
          PLOG_DEBUG << "sort retset, init done, set k = 0";
        }
        break;
      case query_get_data:
        {
          std::vector<unsigned> target_ids;
          while_k = global_index->NewSearchGetData(retset, flags, query_load + query_id * dim, data_load, L, while_k, edge_table_id, target_ids);
          PLOG_DEBUG << "k: " << while_k << " of qe name: " << qe_name;
          std::cout << query_id << "_id's " << "k=" << while_k << std::endl;
          if (while_k == -1){
            query_state = query_finish;
            PLOG_DEBUG << "k>=200 " << qe_name;
            for (size_t i = 0; i < K; i++) {
              PLOG_DEBUG << fmt::format("QE FINISH Query {}'s kNN: For {} is {}",query_id, i, retset[i].id);
            }
            return 1;
          }
          else if (target_ids.empty()){
            PLOG_DEBUG << "k is empty " << qe_name;
            return 1;
            //for this k there is no DC tasks, goto the next k
          }else{
            std::vector<uint64_t> edge_read_list;
            address_mapping(edge_table_id, true, edge_read_list);
            std::vector<QE_MEM_State> edge_read_state;
            for (auto &edge_addr: edge_read_list){
              edge_read_state.push_back(QE_MEM_State{edge_addr, READ_NOT_SEND});
            }
            edge_read_task = QE_DC_Task{qe_name, query_id, edge_table_id, edge_read_state, NOT_SEND};
            for(auto &id : target_ids){
              std::vector<uint64_t> vec_read_list;
              address_mapping(id, false, vec_read_list);
              std::vector<QE_MEM_State> vec_read_state;
              for (auto &vec_addr: vec_read_list){
                vec_read_state.push_back(QE_MEM_State{vec_addr, READ_NOT_SEND});
              }
              dc_task_list.push_back(QE_DC_Task{qe_name, query_id, id, vec_read_state, READ_INFLIGHT}); 
              PLOG_DEBUG << "dc_task_list size: " << dc_task_list.size() << " node_id: " << id;
            }
            
            query_state = query_pending_edge;
            return 1;
          }
            
        }
        break;
      case query_pending_edge://pending the edge table reading
        {
          bool all_edge_read_done = true;
          for (auto &edge_read: edge_read_task.mem_read_state){
            if (edge_read.state != READ_FINISHED){
              all_edge_read_done = false;
              if(!mem_sender.full() && edge_read.state == READ_NOT_SEND){
                mem_sender.push(MemReadTask{edge_read.mem_read_addr, true, qe_name, edge_read_task.node_id, true});
                edge_read.state = READ_INFLIGHT;
                PLOG_DEBUG << fmt::format("Palse task {} to Edge_Table_Reader at cycle: {}", query_id, current_cycle);
                return 1;
              }
              if(!mem_receiver.empty() && edge_read.state == READ_INFLIGHT){
                auto qe_top_addr = mem_receiver.get().addr;
                if (qe_top_addr == edge_read.mem_read_addr){ 
                  mem_receiver.pop();
                  //write into retset
                  edge_read.state = READ_FINISHED;
                  return 1;
                }
              }
            }
          }
          if(all_edge_read_done){
            query_state = query_pending_distance;
            PLOG_DEBUG << "query_pending_edge done";
            return 1;
          }else{
            return 0;
          }
        }
        break;
      case query_pending_distance://pending for dc_task_list
        {
          bool all_distance_ready = true;
          int i=0;
          for (auto &dc_task : dc_task_list){
            //state: READ_INFLIGHT, READ_FINISHED, DC_INFLIGHT, DC_FINISHED
            bool vector_read_done = true;
            for (auto &vec_read: dc_task.mem_read_state){
              if (vec_read.state != READ_FINISHED){
                vector_read_done = false;
                if(!mem_sender.full() && vec_read.state == READ_NOT_SEND){
                  mem_sender.push(MemReadTask{vec_read.mem_read_addr, true, qe_name, dc_task.node_id, false});
                  vec_read.state = READ_INFLIGHT;
                  return 1;
                }
                if(!mem_receiver.empty() && vec_read.state == READ_INFLIGHT){
                  auto qe_top_addr = mem_receiver.get().addr;
                  // auto qe_top_name = mem_receiver.get().qe_name;
                  // auto qe_top_node_id = mem_receiver.get().node_id;
                  if (qe_top_addr == vec_read.mem_read_addr){
                    mem_receiver.pop();
                    vec_read.state = READ_FINISHED;
                    return 1;
                  }
                }
              }
            }
            if(vector_read_done == true && dc_task.state == READ_INFLIGHT){
              dc_task.state = READ_FINISHED;
              PLOG_DEBUG << "dc_task_list_seq: " << i << " Vec Read Finish ";
            }
            PLOG_DEBUG << "dc_task_list_seq: " << i << " node_id: " << dc_task.node_id;
            PLOG_DEBUG << fmt::format("DC_task: {}, state: {}", dc_task.node_id, +dc_task.state);
            if (dc_task.state != DC_FINISHED){
              all_distance_ready = false;
              if(distance_sender.full()){
                PLOG_DEBUG << "distance_sender full";
              }
              if(!distance_sender.full() && dc_task.state == READ_FINISHED){//read finish judge is a vector
                //send dc task
                distance_sender.push(DistanceTask{qe_name, query_id, dc_task.node_id});
                PLOG_DEBUG << fmt::format("QE {} Palse node {} to Distance_Computer at cycle: {}", query_id, dc_task.node_id, current_cycle);
                dc_task.state = DC_INFLIGHT;
                return 1;
              }
              if(return_distance_receiver.empty()){
                PLOG_DEBUG << "return_distance_receiver empty";
              }
              if(!return_distance_receiver.empty() && dc_task.state == DC_INFLIGHT){
                auto top_qename = return_distance_receiver.get().qe_name;
                auto top_query = return_distance_receiver.get().query_id;
                auto top_nodeid = return_distance_receiver.get().node_id;
                if (top_qename == qe_name && top_query == query_id && top_nodeid == dc_task.node_id){
                  return_distance_receiver.pop();
                  dc_task.state = DC_FINISHED;
                  //write into retset
                  PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is done", qe_name, dc_task.query_id);
                  return 1;
                }
              }
            }
            i++;
          }
          if(all_distance_ready){
            PLOG_DEBUG << fmt::format("while_{} is all done", while_k);
            dc_task_list.clear();
            query_state = query_get_data;
            return 1;
          }
          return 0;
        }
        break;
      case query_finish:
        for (size_t i = 0; i < K; i++) {
          PLOG_DEBUG << fmt::format("QE FINISH Query {}'s kNN: For {} is {}",query_id, i, retset[i].id);
        }
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

void near_mem::Query_Engine::address_mapping(unsigned id, bool is_edge_read, std::vector<uint64_t>& address_list){
  int read_size;
  uint64_t start_address;
  if (is_edge_read){ 
    global_index->GetSizeAndAddr(id, read_size, start_address);//read 50*4B ~ 200B
    start_address += (uint64_t)points_num * (uint64_t)dim * (uint64_t)sizeof(float);
  }else{
    read_size = (int)dim; //read dim*4B ~ 400B
    start_address = (uint64_t)(id * dim * sizeof(float));
  }
  read_size += (int)(start_address % CACHELINE_SIZE); //64 is cacheline size
  start_address -= (uint64_t)(start_address % CACHELINE_SIZE); //align start_address to cacheline
  int offset = 0;
  while (read_size > 0){
    address_list.push_back(start_address + CACHELINE_SIZE * offset);
    read_size -= CACHELINE_SIZE;
    offset++;
  }
}

} // namespace near_mem
