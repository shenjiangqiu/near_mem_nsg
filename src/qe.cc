#include <qe.h>

using namespace std;

namespace near_mem {

bool near_mem::Query_Engine::do_cycle() {
  PLOG_DEBUG << "working? " << working() << " state: " << +query_state;
  if (working()) {
    stall_qe++;
    switch(query_state){
      case query_init: //200 init_ids dc request: no need to send mem request
        {
          stall_qe_stage1++;
          if (qe_name == 0){
            stall_qe_stage1_qe0++;
          }
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
            }
          }
          if (dc_ready_all == true){
            query_state = query_init_finish;
            global_index->NewSearchInit(init_ids, retset, query_load + query_id * dim, data_load, L);
            remaining_cycle = 460; //200*log(200): sort cost
            PLOG_DEBUG << "NewSearchInit done";
            cout << "NewSearchInit done" << endl;
            //init for following while loop
          }else{
            stall_dc1++;
          }
          return 0;
        }
        break;

      case query_init_finish:
        //sort retset
        {
          stall_qe_stage2++;
          if (qe_name == 0){
            stall_qe_stage2_qe0++;
          }
          if (remaining_cycle == 0){ // sort cost
            while_k = 0;
            query_state = query_get_data;
            dc_task_list.clear();
            PLOG_DEBUG << "sort retset, init done, set k = 0";
          }else{
            remaining_cycle--;
          }
          return 1;
        }
        break;
      case query_get_data:
        {
          stall_qe_stage3++;
          if (qe_name == 0){
            stall_qe_stage3_qe0++;
          }
          std::vector<unsigned> target_ids;
          std::vector<unsigned> compare_latency;
          if (qe_name == 0)
            std::cout << std::endl << query_id << "_id's k=" << while_k << " " ;
          while_k = global_index->NewSearchGetData(retset, flags, query_load + query_id * dim, data_load, L, while_k, edge_table_id, target_ids, compare_latency, qe_name);
          if(target_ids.size() != compare_latency.size()){
            cout << endl << " error!!" << " task_num: " << target_ids.size() << " compare_num: " << compare_latency.size() << endl;
          }
          PLOG_DEBUG << "k: " << while_k << " of qe name: " << qe_name;
          if (while_k == -1){
            query_state = query_finish;
            PLOG_DEBUG << "k>=200 " << qe_name;
            for (size_t i = 0; i < K; i++) {
              PLOG_DEBUG << fmt::format("QE FINISH Query {}'s kNN: For {} is {}", query_id, i, retset[i].id);
            }
            return 1;
          }
          else if (target_ids.empty()){
            PLOG_DEBUG << "k is empty " << qe_name;
            return 1;
            //for this k there is no DC tasks, goto the next k
          }else{
            offset_edge = 0;
            offset_vec = 0;
            std::vector<uint64_t> edge_read_list;
            address_mapping(edge_table_id, true, edge_read_list, offset_edge);
            std::vector<QE_MEM_State> edge_read_state;
            for (auto &edge_addr: edge_read_list){
              edge_read_state.push_back(QE_MEM_State{edge_addr, READ_NOT_SEND});
            }
            edge_read_task = QE_DC_Task{qe_name, query_id, edge_table_id, edge_read_state, 0, NOT_SEND};
            int i = 0;
            for(auto &id : target_ids){
              std::vector<uint64_t> vec_read_list;
              address_mapping(id, false, vec_read_list, offset_vec);
              std::vector<QE_MEM_State> vec_read_state;
              for (auto &vec_addr: vec_read_list){
                vec_read_state.push_back(QE_MEM_State{vec_addr, READ_NOT_SEND});
              }
              dc_task_list.push_back(QE_DC_Task{qe_name, query_id, id, vec_read_state, (int)compare_latency[i], READ_INFLIGHT}); 
              cout << "i: " << i << " compare_latency " << (int)compare_latency[i];
              i++;
              PLOG_DEBUG << "dc_task_list size: " << dc_task_list.size() << " node_id: " << id;
            }
            target_ids.clear();
            compare_latency.clear();
            if (qe_name == 0)
              std::cout << " offset_edge: " << offset_edge << " offset_vec: " << offset_vec << " 3_4_cycle: " << current_cycle << " ";
            is_first = true;
            first_return = 0;
            last_return = 0;
            sub_is_first = true;
            sub_first_return = 0;
            sub_last_return = 0;
            time_edge_start = current_cycle;
            query_state = query_pending_edge;
            send_edge = 0;
            send_vec = 0;
            rcv_edge = 0;
            rcv_vec = 0;
            return 1;
          }
          
        }
        break;
      case query_pending_edge://pending the edge table reading
        {
          stall_qe_stage4++;
          if (qe_name == 0){
            stall_qe_stage4_qe0++;
          }
          bool all_edge_read_done = true;
          for (auto &edge_read: edge_read_task.mem_read_state){
            if (edge_read.state != READ_FINISHED){
              all_edge_read_done = false;
              if(!mem_sender.full() && edge_read.state == READ_NOT_SEND){
                send_edge++;
                total_edge++;
                total_edge_time -= current_cycle;
                if(qe_name == 0){
                  qe0_edge++;
                  qe0_edge_time -= current_cycle;
                }
                //cout << endl<< "Send_Edge_Addr: " << edge_read.mem_read_addr << " at_cycle: " << current_cycle << endl ;
                mem_sender.push(MemReadTask{edge_read.mem_read_addr, true, qe_name, edge_read_task.node_id, true});
                edge_read.state = READ_INFLIGHT;
                PLOG_DEBUG << fmt::format("Palse task {} to Edge_Table_Reader at cycle: {}", query_id, current_cycle);
                return 1;
              }
              if(!mem_receiver.empty() && edge_read.state == READ_INFLIGHT){
                auto qe_top_addr = mem_receiver.get().addr;
                if (qe_top_addr == edge_read.mem_read_addr){ 
                  rcv_edge++;
                  total_edge_time += current_cycle;
                  if(qe_name == 0){
                    qe0_edge_time += current_cycle;
                  }
                  //cout << endl<< "Receive_Edge_Addr: " << edge_read.mem_read_addr << " at_cycle: " << current_cycle<< endl ;
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
            if (qe_name == 0)
              std::cout << " s1_time: " << current_cycle - time_edge_start;
            exe_time_start = current_cycle;
            PLOG_DEBUG << "query_pending_edge done";
            
            return 1;
          }else{
            stall_mem1++;
            return 0;
          }
        }
        break;
      case query_pending_distance://pending for dc_task_list
        {
          stall_qe_stage5++;
          if (qe_name == 0){
            stall_qe_stage5_qe0++;
          }
          bool all_vec_read_ready = true;
          bool all_distance_ready = true;
          bool all_sort_ready = true;
          bool compare_busy = false;
          int i=0;
          // int count_finish_sort = 0;
          for (auto &dc_task : dc_task_list){
            //state: READ_INFLIGHT, READ_FINISHED, DC_INFLIGHT, DC_FINISHED
            bool vector_read_done = true;
            for (auto &vec_read: dc_task.mem_read_state){
              if (vec_read.state != READ_FINISHED){
                vector_read_done = false;
                all_vec_read_ready = false;
                stall_mem++;
                if(!mem_sender.full() && vec_read.state == READ_NOT_SEND){
                  mem_sender.push(MemReadTask{vec_read.mem_read_addr, true, qe_name, dc_task.node_id, false});
                  send_vec++;
                  total_vec++;
                  total_vec_time -= current_cycle;
                  //cout << endl<< "Send_Vec_Addr: " << vec_read.mem_read_addr << " at_cycle: " << current_cycle << endl;
                  if(qe_name == 0){
                    qe0_vec++;
                    qe0_vec_time -= current_cycle;
                  }
                  stall_mem3++;
                  vec_read.state = READ_INFLIGHT;
                  return 1;
                }
                if(!mem_receiver.empty() && vec_read.state == READ_INFLIGHT){
                  auto qe_top_addr = mem_receiver.get().addr;
                  if(sub_is_first){
                    sub_is_first = false;
                    sub_first_return = current_cycle;
                  }
                  sub_last_return = current_cycle;
                  // auto qe_top_name = mem_receiver.get().qe_name;
                  // auto qe_top_node_id = mem_receiver.get().node_id;
                  if (qe_top_addr == vec_read.mem_read_addr){
                    mem_receiver.pop();
                    // cout << "vec ready" << endl;
                    //cout << endl<< "Receive_Vec_Addr: " << vec_read.mem_read_addr << " at_cycle: " << current_cycle << endl;
                    rcv_vec++;
                    total_vec_time += current_cycle;
                    if(qe_name == 0){
                      qe0_vec_time += current_cycle;
                    }
                    stall_mem3++;
                    vec_read.state = READ_FINISHED;
                    return 1;
                  }
                }
              }
            }
            if(vector_read_done == true && dc_task.state == READ_INFLIGHT){
              dc_task.state = READ_FINISHED;
              if(is_first){
                is_first = false;
                first_return = current_cycle;
              }
              last_return = current_cycle;
              PLOG_DEBUG << "dc_task_list_seq: " << i << " Vec Read Finish ";
            }
            PLOG_DEBUG << "dc_task_list_seq: " << i << " node_id: " << dc_task.node_id;
            PLOG_DEBUG << fmt::format("DC_task: {}, state: {}", dc_task.node_id, +dc_task.state);
            if (dc_task.state != DC_FINISHED && dc_task.state != SORT_FINISHED){
              all_distance_ready = false;
              if(all_vec_read_ready)
                stall_dc++;
              if(distance_sender.full()){
                PLOG_DEBUG << "distance_sender full";
              }
              if(!distance_sender.full() && dc_task.state == READ_FINISHED){//read finish judge is a vector
                //send dc task
                send_dc++;
                total_dc_time -= current_cycle;
                distance_sender.push(DistanceTask{qe_name, query_id, dc_task.node_id});
                PLOG_DEBUG << fmt::format("QE {} Palse node {} to Distance_Computer at cycle: {}", query_id, dc_task.node_id, current_cycle);
                dc_task.state = DC_INFLIGHT;
                stall_mem3++;
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
                  rcv_dc++;
                  total_dc_time += current_cycle;
                  dc_task.state = DC_FINISHED;
                  // cout << "dc ready" << endl;
                  //write into retset
                  stall_mem3++;
                  PLOG_DEBUG << fmt::format("QE name {}: palse_id {} is done", qe_name, dc_task.query_id);
                  return 1;
                }
              }
            } 
            if(dc_task.state == DC_FINISHED) { // finish dc (DC_FINISHED state), get into sorting
              if (dc_task.remaining_compare_cycle == 0) {
                dc_task.state = SORT_FINISHED;
                // count_finish_sort++;
                // cout << "count_finish_sort" << i << endl;
              } else {
                all_sort_ready = false;
                if(compare_busy == false){
                  compare_busy = true; // sort cannot be parallel inside one cycle
                  if(all_distance_ready == true){
                    stall_mem4++;
                  }
                  dc_task.remaining_compare_cycle--;
                }
              }
            }
            i++;
          }
          // cout << "count_finish_sort" << count_finish_sort << endl;
          if(all_sort_ready && all_distance_ready){
            // cout << "5 ready" << endl;
            if(remaining_cycle == 0){ //compare latency
              PLOG_DEBUG << fmt::format("while_{} is all done", while_k);
              mem_receive_delta += last_return - first_return;
              mem_receive_count++;
              sub_mem_receive_delta += sub_last_return - sub_first_return;
              sub_mem_receive_count++;
              if (qe_name == 0){
                std::cout << " send_edge: " << send_edge << " send_vec: " << send_vec << " ";
                std::cout << " rcv_edge: " << rcv_edge << " rcv_vec: " << rcv_vec << " ";
                std::cout << " total_edge_time: " << total_edge_time << " total_vec_time: " << total_vec_time << " ";
                std::cout << " avg_total_edge_time: " << (double)total_edge_time/total_edge << " avg_total_vec_time: " << (double)total_vec_time/total_vec << " ";
                std::cout << " qe0_edge: " << qe0_edge << " qe0_vec: " << qe0_vec << " ";
                std::cout << " qe0_edge_time: " << qe0_edge_time << " qe0_vec_time: " << qe0_vec_time << " ";
                std::cout << " total_dc_send: " << send_dc << " total_dc_rcv: " << rcv_dc << " ";
                std::cout << " total_dc_time: " << total_dc_time << " avg_dc_time: " << (double)total_dc_time/send_dc << " ";
                std::cout << " avg_qe0_edge_time: " << (double)qe0_edge_time/qe0_edge << " avg_qe0_vec_time: " << (double)qe0_vec_time/qe0_vec << " ";
                std::cout << " exe_time_delta: " << current_cycle - exe_time_start << " with_unfold_num: " << dc_task_list.size();
              }
              unfold_num += dc_task_list.size();
              exe_time += current_cycle - exe_time_start;
              exe_time_start = 0;
              dc_task_list.clear();
              query_state = query_get_data;
              return 1;
            }else{
              remaining_cycle--;
              stall_mem3++;
              return 1;
            }
          } else{
            stall_mem2++;
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

    PLOG_DEBUG << "QE name " << qe_name << " state: " << +query_state << " current_cycle: " << current_cycle;
    return 1;
  } else { //QE is not working, fetch new task
    stall_qe_stage7++;
    if (qe_name == 0){
      stall_qe_stage7_qe0++;
    }
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

void near_mem::Query_Engine::address_mapping(unsigned id, bool is_edge_read, std::vector<uint64_t>& address_list, int& accumulate_offset){
  int read_size;
  uint64_t start_address;
  if (is_edge_read){ 
    global_index->GetSizeAndAddr(id, read_size, start_address);//read 50*4B ~ 200B
    start_address += (uint64_t)points_num * (uint64_t)dim * (uint64_t)global_sizeof;
  }else{
    read_size = (int)dim; //read dim*4B ~ 400B
    start_address = (uint64_t)id * (uint64_t)dim * (uint64_t)global_sizeof;
  }
  read_size += (int)(start_address % CACHELINE_SIZE); //64 is cacheline size
  start_address -= (uint64_t)(start_address % CACHELINE_SIZE); //align start_address to cacheline
  int offset = 0;
  while (read_size > 0){
    address_list.push_back(start_address + CACHELINE_SIZE * offset);
    read_size -= CACHELINE_SIZE;
    offset++;
  }
  accumulate_offset += offset;
}

} // namespace near_mem
