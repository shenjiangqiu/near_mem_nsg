#include <PrimitiveBloomFilter.h>
#include <RamulatorConfig.h>
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <chrono>
#include <component.h>
#include <controller.h>
#include <ctime>
#include <dc.h>
#include <efanna2e/exceptions.h>
#include <efanna2e/index_nsg.h>
#include <efanna2e/parameters.h>
#include <efanna2e/util.h>
#include <fmt/format.h>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <qe.h>
#include <ramulator_wrapper.h>
#include <sstream>
#include <struct.h>
#include <task_queue.h>
#include <time.h>
#include <toml.hpp>
#include <vector>

using namespace std;
using namespace std::chrono;
using namespace near_mem;

#define N 1
#define M 1

unsigned L = 200;
unsigned K = 100;
unsigned dimension, points_num, dim, query_num, query_dim;
unsigned num_compute_unit = 64;
unsigned latency_compute_unit = 1;
float* data_load = NULL;
float* query_load = NULL;
std::vector<efanna2e::Neighbor> global_retset;
std::vector<unsigned> global_init_ids;
boost::dynamic_bitset<> global_flags;
PrimitiveBloomFilter<unsigned,80000> global_BF(8000,10);
efanna2e::IndexNSG* global_index;

void load_data(std::string filename, float*& data, unsigned& num, unsigned& dim) {  // load data with sift10K pattern
  std::ifstream in(filename, std::ios::binary);
  if (!in.is_open()) {
    std::cout << "open file error" << std::endl;
    exit(-1);
  }
  in.read((char*)&dim, 4);
  std::cout << "data dimension: " << dim << std::endl;
  in.seekg(0, std::ios::end);
  std::ios::pos_type ss = in.tellg();
  size_t fsize = (size_t)ss;

  std::cout<<"fsize: "<<(u_int64_t)fsize<< " filename: " << filename << std::endl;
  
  num = (u_int64_t)(fsize / (dim + 1) / 4);

  auto size=num * (u_int64_t)dim;
  data = new float[size];
  
  in.seekg(0, std::ios::beg);
  for (u_int64_t i = 0; i < num; i++) {
    in.seekg(4, std::ios::cur);
    in.read((char*)(data + (u_int64_t)i * dim), dim * 4);
  }
  in.close();
}

void save_result(std::string filename, std::vector<std::vector<unsigned> >& results) {
  std::ofstream out(filename, std::ios::binary | std::ios::out);

  for (unsigned i = 0; i < results.size(); i++) {
    unsigned GK = (unsigned)results[i].size();
    out.write((char*)&GK, sizeof(unsigned));
    out.write((char*)results[i].data(), GK * sizeof(unsigned));
  }
  out.close();
}

int main(int argc, char **argv) {
  uint64_t current_cycle = 0;
  
  std::cout << "*******Read Config: *******" << std::endl;
  
    // read config file path, for example: "./config.toml"
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
    return 1;
  }
  auto config_path = argv[1];
  toml::table tbl;
  // build the config table
  try {
    tbl = toml::parse_file(config_path);
    std::cout << tbl << "\n";
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  // get the config value
  auto bench_num=tbl["programs_num"].ref<int64_t>();
  std::cout << "choose bench num: " << bench_num<< std::endl;
  auto type = tbl["query_type"][bench_num].ref<std::string>();
  std::cout << "query_type: " << type << std::endl;
  auto path = tbl["path"].ref<std::string>();
  auto base_name = tbl["base"][bench_num].ref<std::string>();
  auto query_name = tbl["query"][bench_num].ref<std::string>();
  auto nsg_name = tbl["nsg"][bench_num].ref<std::string>();
  std::string base_file = path + "/base/" + base_name + ".fvecs";
  std::string query_file = path + "/query/"  + query_name + ".fvecs";
  std::string nsg_file = path + "/nsg/" + nsg_name + ".nsg";
  std::string result_file = tbl["result_path"].ref<std::string>() + nsg_name + \
                            ".search" + tbl["result_no"].ref<std::string>() + ".ivecs";
  time_t timestamp;
  std::stringstream ss;
  ss << time(&timestamp);
  std::string ts = ss.str();
  std::string debug_file = tbl["result_path"].ref<std::string>() + "test_" + ts + ".log";
  std::cout << "base_file: " << base_file << std::endl;
  std::cout << "query_file: " << query_file << std::endl;
  std::cout << "nsg_file: " << nsg_file << std::endl;
  std::cout << "result_file: " << result_file << std::endl;
  L = (unsigned)tbl["L"].ref<int64_t>();
  K = (unsigned)tbl["K"].ref<int64_t>();
  std::cout << "L: " << L << " K: " << K << std::endl;
  if (L < K) {
    std::cout << "search_L cannot be smaller than search_K!" << std::endl;
    assert(0);
  }
  load_data(base_file, data_load, points_num, dim);
  load_data(query_file, query_load, query_num, query_dim);
  assert(dim == query_dim);
  efanna2e::IndexNSG index(dim, points_num, efanna2e::L2, nullptr);
  index.Load(nsg_file);
  efanna2e::Parameters paras;
  paras.Set<unsigned>("L_search", L);
  paras.Set<unsigned>("P_search", L);
  std::cout << std::fixed << std::setprecision(9) << std::left;
  // auto s = std::chrono::high_resolution_clock::now();
  // auto e = std::chrono::high_resolution_clock::now();
  //TODO: pre-process for each query
  global_retset = std::vector<efanna2e::Neighbor>(L + 1);
  global_init_ids = std::vector<unsigned>(L);
  global_flags = boost::dynamic_bitset<>{points_num, 0};
  // PrimitiveBloomFilter<unsigned,80000> BF(8000,10);
  index.PreProcess(global_init_ids, global_BF, global_flags, L);
  // index.init_graph(paras);
  
  global_index = &index;
  
  // query_num = 10;

  plog::init(plog::debug, debug_file.c_str(), 64 * 1024 * 1024, 10);
  std::cout << "*******Start Sim: *******" << std::endl;
  unsigned num_qe = 8;
  unsigned num_dc = 32;
  auto [mem_sender, mem_receiver] = make_task_queue<MemReadTask>(64);
  auto [mem_ret_sender, mem_ret_receiver] = make_task_queue<MemReadTask>(64);
  auto [nsg_sender, nsg_receiver] = make_task_queue<NsgTask>(4);
  
  auto controller = Controller(
    current_cycle, 
    nsg_sender,
    mem_sender, 
    mem_ret_receiver, 
    dim, 
    points_num,
    efanna2e::Metric::L2, 
    nullptr, 
    nullptr, 
    1000 //query_num
    );

  auto [dist_sender, dist_receiver] = make_task_queue<DistanceTask>(4);
  auto [return_dist_sender, return_dist_receiver] = make_task_queue<DistanceTask>(4);
  std::vector<Query_Engine> qes;
  for (auto i = 0u; i < num_qe; ++i) {
    qes.push_back(Query_Engine(
      current_cycle, 
      i,
      nsg_receiver, 
      dist_sender, 
      return_dist_receiver,
      mem_sender,
      mem_ret_receiver
      ));
  }
  std::vector<Distance_Computer> dcs;
  for (auto i = 0u; i < num_dc; ++i) {
    dcs.push_back(Distance_Computer(
      current_cycle,
      i,
      dist_receiver, 
      return_dist_sender
      ));
  }

  ramulator::Config m_config("DDR4-config.cfg");
  m_config.set_core_num(1);
  auto mem = ramulator_wrapper(
    m_config, 
    64, 
    current_cycle, 
    std::move(mem_receiver),
    std::move(mem_ret_sender));

  std::vector<Component *> components;
  components.push_back(&controller);
  for (auto &qe : qes) {
    components.push_back(&qe);
  }
  for (auto &dc : dcs) {
    components.push_back(&dc);
  }
  components.push_back(&mem);
  int DEAD_LOCK = 0;
  while (true) {
    DEAD_LOCK++;
    if (DEAD_LOCK > 10000000) {
      PLOG_DEBUG << "dead lock";
      break;
    }
    for (auto component : components) {
      component->cycle();
      current_cycle++;//todo: ALL Component run parallel
    }
    bool all_end = true;

    for (auto component : components) {
      if (!component->all_end()) {
        all_end = false;
        break;
      }
    }
    if (all_end) {
      break;
    }
  }
  PLOG_DEBUG << fmt::format("Simulation Finished at cycle: {}, {}", current_cycle, DEAD_LOCK);

  return 0;
}
