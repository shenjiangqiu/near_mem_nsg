//
// Created by 付聪 on 2017/6/21.
//

#include <efanna2e/index_nsg.h>
#include <efanna2e/util.h>
#include <omp.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
#include <chrono>
#include <ctime>
#define N 1
using namespace std;
using namespace std::chrono;

void load_data(char* filename, float*& data, unsigned& num,
               unsigned& dim) {  // load data with sift10K pattern
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

void save_result(char* filename, std::vector<std::vector<unsigned> >& results) {
  std::ofstream out(filename, std::ios::binary | std::ios::out);

  for (unsigned i = 0; i < results.size(); i++) {
    unsigned GK = (unsigned)results[i].size();
    out.write((char*)&GK, sizeof(unsigned));
    out.write((char*)results[i].data(), GK * sizeof(unsigned));
  }
  out.close();
}
int main(int argc, char** argv) {
  if (argc != 7) {
    std::cout << argv[0]
              << " data_file query_file nsg_path search_L search_K result_path"
              << std::endl;
    exit(-1);
  }
  float* data_load = NULL;
  unsigned points_num, dim;
  load_data(argv[1], data_load, points_num, dim);
  float* query_load = NULL;
  unsigned query_num, query_dim;
  load_data(argv[2], query_load, query_num, query_dim);
  assert(dim == query_dim);

  unsigned L = (unsigned)atoi(argv[4]);
  unsigned K = (unsigned)atoi(argv[5]);

  if (L < K) {
    std::cout << "search_L cannot be smaller than search_K!" << std::endl;
    exit(-1);
  }

  // data_load = efanna2e::data_align(data_load, points_num, dim);//one must
  // align the data before build query_load = efanna2e::data_align(query_load,
  // query_num, query_dim);
  efanna2e::IndexNSG index(dim, points_num, efanna2e::L2, nullptr);
  index.Load(argv[3]);

  efanna2e::Parameters paras;
  paras.Set<unsigned>("L_search", L);
  paras.Set<unsigned>("P_search", L);
  std::cout << std::fixed << std::setprecision(9) << std::left;
  auto s = std::chrono::high_resolution_clock::now();
  auto e = std::chrono::high_resolution_clock::now();
  std::vector<std::vector<unsigned> > res[N];

  // query_num = 10;
  // index.init_graph(paras);
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
        index.Search(query_load + i * dim, data_load, K, paras, tmp.data(), true);
      }
      else
        index.Search(query_load + i * dim, data_load, K, paras, tmp.data(), false);
      res[j].push_back(tmp);
    }
    if (j == 0) {
      e = std::chrono::high_resolution_clock::now();
    }
  }
  std::chrono::duration<double> diff = e - s;
  std::cout << "search time: " << diff.count() << "\n";

  save_result(argv[6], res[0]);

  return 0;
}
