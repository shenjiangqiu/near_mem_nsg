#define TOML_HEADER_ONLY 1
#include <fmt/format.h>
#include <iostream>
#include <toml.hpp>
#include <efanna2e/index_nsg.h>
#include <efanna2e/util.h>
#include <efanna2e/exceptions.h>
#include <efanna2e/parameters.h>
#include <PrimitiveBloomFilter.h>
#include <boost/dynamic_bitset.hpp>
#include <bitset>
#include <omp.h>
#include <iomanip>
#include <vector>
#include <numeric>
#include <chrono>
#include <ctime>
#include <struct.h>
#define N 1
#define M 1
using namespace std;
using namespace std::chrono;

unsigned dimension=0;

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
  std::cout << "base_file: " << base_file << std::endl;
  std::cout << "query_file: " << query_file << std::endl;
  std::cout << "nsg_file: " << nsg_file << std::endl;
  std::cout << "result_file: " << result_file << std::endl;
  unsigned L = (unsigned)tbl["L"].ref<int64_t>();
  unsigned K = (unsigned)tbl["K"].ref<int64_t>();
  // auto BF_bit = tbl["BF_bit"].ref<int64_t>();
  // auto BF_hash = tbl["BF_hash"].ref<int64_t>();
  // auto BF_unit = tbl["BF_unit"].ref<int64_t>();
  
  std::cout << "L: " << L << " K: " << K << std::endl;
  if (L < K) {
    std::cout << "search_L cannot be smaller than search_K!" << std::endl;
    exit(-1);
  }
  float* data_load = NULL;
  float* query_load = NULL;
  unsigned points_num, dim, query_num, query_dim;
  load_data(base_file, data_load, points_num, dim);
  load_data(query_file, query_load, query_num, query_dim);
  assert(dim == query_dim);
  dimension = dim;
  // data_load = efanna2e::data_align(data_load, points_num, dim);//one must
  // align the data before build query_load = efanna2e::data_align(query_load,
  // query_num, query_dim);
  efanna2e::IndexNSG index(dim, points_num, efanna2e::L2, nullptr);
  index.Load(nsg_file);

  efanna2e::Parameters paras;
  paras.Set<unsigned>("L_search", L);
  paras.Set<unsigned>("P_search", L);
  std::cout << std::fixed << std::setprecision(9) << std::left;
  auto s = std::chrono::high_resolution_clock::now();
  auto e = std::chrono::high_resolution_clock::now();
  //TODO: pre-process for each query
  std::vector<efanna2e::Neighbor> retset(L + 1);
  std::vector<unsigned> init_ids(L);
  PrimitiveBloomFilter<unsigned,80000> BF(8000,10);
  boost::dynamic_bitset<> flags{points_num, 0};
  index.PreProcess(init_ids, BF, flags, L);
  // std::cout << "init_ids: " << std::endl;
  // for (unsigned kk = 0; kk < K; kk++) {
  //   std::cout << tmp1[kk] << std::endl;
  // }
  // std::cout << "init_ids_finish" << std::endl;
  // index.Print_Edge_Vec();
  std::vector<std::vector<unsigned>> res[N];
  // query_num = 10;
  // 
  query_num = 1000;
  for (unsigned k = 0; k < M; k++){
#pragma omp parallel for schedule(dynamic)
    for (unsigned j = 0; j < N; j++){
      if (j == 0){
        s = std::chrono::high_resolution_clock::now();
      }
      // std::cout << "user: "<< j << " thread: " << omp_get_thread_num() << std::endl;
      for (unsigned count = 0, i = j*query_num/N; count < query_num; i++, i%=query_num, count++){
        std::vector<unsigned> tmp(K);
        if (j == 0){
          std::cout << "*****************************************" << std::endl;
          std::cout << "Query: " << i << " " << std::endl;
          auto new_BF=BF;
          auto new_flags=flags;
          // std::vector<unsigned> tmp(K);
          index.NewSearch(init_ids, new_BF, new_flags, query_load + i * dim, data_load, K, L, tmp.data(), true);
          // index.Search(query_load + i * dim, data_load, K, paras, tmp.data(), true);
          // if (i == 0){
          //   std::cout << "init_ids_x: " << std::endl;
          //   for (unsigned kk = 0; kk < K; kk++) {
          //     std::cout << tmp[kk] << std::endl;
          //   }
          //   std::cout << "init_ids_finish_x" << std::endl;
          // }
        }
        else
          index.Search(query_load + i * dim, data_load, K, paras, tmp.data(), false);
        res[j].push_back(tmp);
      }
      if (j == 0) {
        e = std::chrono::high_resolution_clock::now();
      }
    }
  }
  std::chrono::duration<double> diff = e - s;
  std::cout << "search time: " << diff.count() << "\n";

  save_result(result_file, res[0]);

  return 0;

}