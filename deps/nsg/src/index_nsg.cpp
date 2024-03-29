#include "efanna2e/index_nsg.h"
#include "PrimitiveBloomFilter.h"
#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <omp.h>
#include <vector>

#include "efanna2e/exceptions.h"
#include "efanna2e/parameters.h"

// #define BLOOM_FILTER
#define EARLY_TERMINATION

using namespace std;
using namespace std::chrono;

namespace efanna2e {
#define _CONTROL_NUM 100
IndexNSG::IndexNSG(const size_t dimension, const size_t n, Metric m, Index *initializer)
        : Index(dimension, n, m), initializer_{initializer} {}
IndexNSG::~IndexNSG() {}

void IndexNSG::Search(const float *query, const float *x, size_t K, 
                      const Parameters &parameters, unsigned *indices, bool thread_zero) 
{
  auto s1 = std::chrono::high_resolution_clock::now();
  // std::cout << "s1: " << std::chrono::duration_cast<std::chrono::milliseconds>(s1.time_since_epoch()).count() %10000<< std::endl;
  
#ifdef BLOOM_FILTER
  PrimitiveBloomFilter<unsigned,80000> BF(8000,10);
#else
  boost::dynamic_bitset<> flags{nd_, 0};
#endif
  const unsigned L = parameters.Get<unsigned>("L_search");
  data_ = x;
  u_int64_t neighbor_count = 0;
  u_int64_t neighbor_count1 = 0;
  u_int64_t neighbor_count2 = 0;
  u_int64_t count1 = 0;
  u_int64_t count2 = 0;
  u_int64_t count3 = 0;
  u_int64_t count4 = 0;
  u_int64_t count5 = 0;
  u_int64_t jump_count = 0;
  u_int64_t test1 = 0;
  u_int64_t test2 = 0;
  u_int64_t test_flag = 0;
  u_int64_t insert_count = 0;
  std::vector<Neighbor> retset(L + 1);
  std::vector<unsigned> init_ids(L);
  
  // std::mt19937 rng(rand());
  // GenRandom(rng, init_ids.data(), L, (unsigned) nd_);
  // auto s2 = std::chrono::high_resolution_clock::now();
  // std::cout << "s2: " << std::chrono::duration_cast<std::chrono::milliseconds>(s2.time_since_epoch()).count() %10000<< std::endl;
  
  // cout << "L: " << L << endl;
  // cout << "final_graph_[ep_].size()" << final_graph_[ep_].size() << endl;

  unsigned tmp_l = 0;
  for (; tmp_l < final_graph_[ep_].size(); tmp_l++) {
    init_ids[tmp_l] = final_graph_[ep_][tmp_l];
#ifdef BLOOM_FILTER
    BF.addElement(init_ids[tmp_l]);
#else
    flags[init_ids[tmp_l]] = true;
#endif
    test_flag++;
  }
  for (unsigned i = 0; i < final_graph_[ep_].size(); i++){
    unsigned neigh = final_graph_[ep_][i];
    for (unsigned j = 0; j < final_graph_[neigh].size(); j++){
      unsigned temp_id = final_graph_[neigh][j];
#ifdef BLOOM_FILTER
      if (!BF.containsElement(temp_id)){
        init_ids[tmp_l] = temp_id;
        BF.addElement(temp_id);
        test_flag++;
        tmp_l++;
      }
#else
      if (flags[temp_id] == false){
        init_ids[tmp_l] = temp_id;
        flags[temp_id] = true;
        test_flag++;
        tmp_l++;
      }
#endif
      if (tmp_l == L)
        break;
    }
    if (tmp_l == L)
      break;
  }

  while (tmp_l < L) {
    unsigned id = rand() % nd_;
#ifdef BLOOM_FILTER
        if (BF.containsElement(id))
          continue;
        BF.addElement(id);
#else
        if (flags[id]) 
          continue;
        flags[id] = true;
#endif
    test_flag++;
    init_ids[tmp_l] = id;
    tmp_l++;
  }

  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    float dist = distance_->compare(data_ + dimension_ * id, query, (unsigned)dimension_);
    neighbor_count1++;
    retset[i] = Neighbor(id, dist, true);
    // flags[id] = true;
  }
  std::sort(retset.begin(), retset.begin() + L);
  //Early Termination
  bool Early_Term_flag = false;
  u_int64_t Early_Term_Counter = 0;
  int k = 0;
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;

      for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
        unsigned id = final_graph_[n][m];
#ifdef BLOOM_FILTER
        if (BF.containsElement(id))
          continue;
        BF.addElement(id);
#else
        if (flags[id]) 
          continue;
        flags[id] = true;
#endif
        float dist = distance_->compare(query, data_ + dimension_ * id, (unsigned)dimension_);
        neighbor_count2++;
        if (dist >= retset[L - 1].distance) {
          jump_count++;
          continue;
        }
        Neighbor nn(id, dist, true);
        int r = InsertIntoPool(retset.data(), L, nn);
        insert_count++;
#ifdef EARLY_TERMINATION
        if (r >= 150) {
          Early_Term_Counter++;
        } else {
          Early_Term_Counter = 0;
        }
        if(Early_Term_Counter >= 10){
          Early_Term_flag = true;
        }
#endif
        // if (thread_zero){
        //   std::cout << "Insert_" << insert_count << " r= " << r << std::endl;
        // }
        if (r < nk) {
          nk = r;
          count1++;
        }
      }
    }
    if (nk <= k){
      k = nk;
      count2++;
    }
    else{
      count3++;
      k++;
    }
#ifdef EARLY_TERMINATION
    if (Early_Term_flag){
      count4++;
      break;
    }
#endif
  }
  
  for (size_t i = 0; i < K; i++) {
    indices[i] = retset[i].id;
  }
  auto s4 = std::chrono::high_resolution_clock::now();
  // std::cout << "s4: " << std::chrono::duration_cast<std::chrono::milliseconds>(s4.time_since_epoch()).count() %10000<< std::endl;;
  if (thread_zero){
    // std::cout << "neighbor_count: " << neighbor_count1 + neighbor_count2 << std::endl;
    // std::cout << "neighbor_count1: " << neighbor_count1 << std::endl;
    std::cout << "neighbor_count2: " << neighbor_count2 << std::endl;
    std::cout << "Insert_count: " << insert_count << std::endl;
    // std::cout << "jump1_count: " << count1 << std::endl;
    // std::cout << "jump2_count: " << count2 << std::endl;
    // std::cout << "jump3_count: " << count3 << std::endl;
    std::cout << "Early_Term_count: " << count4 << std::endl;
    std::cout << "quick_count: " << test1 << std::endl;
    std::cout << "sloww_count: " << test2 << std::endl;
    std::cout << "jump_count: " << jump_count << std::endl;
    std::cout << "test_flag: " << test_flag << std::endl;
    // std::cout << "stime1: " << std::chrono::duration_cast<std::chrono::milliseconds>(s2 - s1).count() << " ms" << std::endl;
    // std::cout << "stime2: " << std::chrono::duration_cast<std::chrono::milliseconds>(s4 - s3).count() << " ms" << std::endl;
    // std::cout << "Total_time1: " << std::chrono::duration_cast<std::chrono::milliseconds>(s4 - s1).count() << " ms" << std::endl;
    std::cout << "Total_time2: " << std::chrono::duration_cast<std::chrono::microseconds>(s4 - s1).count() << " us" << std::endl;
  }
}

void IndexNSG::PreProcess(
  std::vector<unsigned>& init_ids, 
  PrimitiveBloomFilter<unsigned,80000>& BF, 
  boost::dynamic_bitset<>& flags,
  size_t L){
  u_int64_t test_flag = 0;
  unsigned tmp_l = 0;
  for (; tmp_l < final_graph_[ep_].size(); tmp_l++) {
    init_ids[tmp_l] = final_graph_[ep_][tmp_l];
#ifdef BLOOM_FILTER
    BF.addElement(init_ids[tmp_l]);
#else
    BF.addElement(init_ids[tmp_l]);
    flags[init_ids[tmp_l]] = true;
#endif
    test_flag++;
  }
  for (unsigned i = 0; i < final_graph_[ep_].size(); i++){
    unsigned neigh = final_graph_[ep_][i];
    for (unsigned j = 0; j < final_graph_[neigh].size(); j++){
      unsigned temp_id = final_graph_[neigh][j];
#ifdef BLOOM_FILTER
      if (!BF.containsElement(temp_id)){
        init_ids[tmp_l] = temp_id;
        BF.addElement(temp_id);
        test_flag++;
        tmp_l++;
      }
#else
      if (flags[temp_id] == false){
        init_ids[tmp_l] = temp_id;
        flags[temp_id] = true;
        test_flag++;
        tmp_l++;
      }
#endif
      if (tmp_l == L)
        break;
    }
    if (tmp_l == L)
      break;
  }

  while (tmp_l < L) {
    unsigned id = rand() % nd_;
#ifdef BLOOM_FILTER
        if (BF.containsElement(id))
          continue;
        BF.addElement(id);
#else
        if (flags[id]) 
          continue;
        flags[id] = true;
#endif
    test_flag++;
    init_ids[tmp_l] = id;
    tmp_l++;
  }
  std::cout << "test_flag: " << test_flag << std::endl;
}

void IndexNSG::NewSearch(
      std::vector<unsigned>& init_ids_init,
      PrimitiveBloomFilter<unsigned,80000>& BF,
      boost::dynamic_bitset<>& flags,
      const float *query,
      const float *base,
      size_t K,
      size_t L,
      unsigned *indices,
      bool thread_zero)
{
  data_ = base;
  u_int64_t neighbor_count = 0;
  u_int64_t neighbor_count1 = 0;
  u_int64_t neighbor_count2 = 0;
  u_int64_t count1 = 0;
  u_int64_t count2 = 0;
  u_int64_t count3 = 0;
  u_int64_t count4 = 0;
  u_int64_t count5 = 0;
  u_int64_t jump_count = 0;
  u_int64_t test1 = 0;
  u_int64_t test2 = 0;
  u_int64_t insert_count = 0;
  auto s1 = std::chrono::high_resolution_clock::now();
  
  // PrimitiveBloomFilter<unsigned,80000> BF(8000,10);
  // BF = BF_init;
  std::vector<Neighbor> retset(L + 1);
  // retset = retset_init;
  std::vector<unsigned> init_ids(L);
  init_ids = init_ids_init;
  
  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    float dist = distance_->compare(data_ + dimension_ * id, query, (unsigned)dimension_);
    neighbor_count1++;
    retset[i] = Neighbor(id, dist, true);
    // flags[id] = true;
  }
  std::sort(retset.begin(), retset.begin() + L);
  //Early Termination
  bool Early_Term_flag = false;
  u_int64_t Early_Term_Counter = 0;
  int k = 0;
  while (k < (int)L) {
    cout << "k: " << k << endl;
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;
      //TODO: read edge table
      // std::cout << "id: " << n << " edgetable_size = " << final_graph_[n].size() << std::endl;

      for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
        unsigned id = final_graph_[n][m];
#ifdef BLOOM_FILTER
        if (BF.containsElement(id))
          continue;
        BF.addElement(id);
#else
        if (flags[id]) 
          continue;
        flags[id] = true;
        BF.addElement(id);
#endif
        float dist = distance_->compare(query, data_ + dimension_ * id, (unsigned)dimension_);
        neighbor_count2++;
        if (dist >= retset[L - 1].distance) {
          jump_count++;
          continue;
        }
        Neighbor nn(id, dist, true);
        int r = InsertIntoPool(retset.data(), L, nn);
        insert_count++;
#ifdef EARLY_TERMINATION
        if (r >= 150) {
          Early_Term_Counter++;
        } else {
          Early_Term_Counter = 0;
        }
        if(Early_Term_Counter >= 10){
          Early_Term_flag = true;
        }
#endif
        // if (thread_zero){
        //   std::cout << "Insert_" << insert_count << " r= " << r << std::endl;
        // }
        if (r < nk) {
          nk = r;
          count1++;
        }
      }
      
    }
    if (nk <= k){
      k = nk;
      count2++;
    }
    else{
      count3++;
      k++;
    }
#ifdef EARLY_TERMINATION
    if (Early_Term_flag){
      count4++;
      break;
    }
#endif
  }
  std::cout << "######################" << std::endl;
  for (size_t i = 0; i < K; i++) {
    indices[i] = retset[i].id;
    cout << "indices[" << i << "] = " << indices[i] << endl;
  }
  auto s4 = std::chrono::high_resolution_clock::now();
  // std::cout << "s4: " << std::chrono::duration_cast<std::chrono::milliseconds>(s4.time_since_epoch()).count() %10000<< std::endl;;
  if (thread_zero){
    // std::cout << "neighbor_count: " << neighbor_count1 + neighbor_count2 << std::endl;
    // std::cout << "neighbor_count1: " << neighbor_count1 << std::endl;
    std::cout << "neighbor_count2: " << neighbor_count2 << std::endl;
    std::cout << "Insert_count: " << insert_count << std::endl;
    // std::cout << "jump1_count: " << count1 << std::endl;
    // std::cout << "jump2_count: " << count2 << std::endl;
    // std::cout << "jump3_count: " << count3 << std::endl;
    std::cout << "Early_Term_count: " << count4 << std::endl;
    std::cout << "quick_count: " << test1 << std::endl;
    std::cout << "sloww_count: " << test2 << std::endl;
    std::cout << "jump_count: " << jump_count << std::endl;
    
    // std::cout << "stime1: " << std::chrono::duration_cast<std::chrono::milliseconds>(s2 - s1).count() << " ms" << std::endl;
    // std::cout << "stime2: " << std::chrono::duration_cast<std::chrono::milliseconds>(s4 - s3).count() << " ms" << std::endl;
    // std::cout << "Total_time1: " << std::chrono::duration_cast<std::chrono::milliseconds>(s4 - s1).count() << " ms" << std::endl;
    std::cout << "Total_time2: " << std::chrono::duration_cast<std::chrono::microseconds>(s4 - s1).count() << " us" << std::endl;
  }
}


void IndexNSG::NewSearchInit(
      std::vector<unsigned>& init_ids,
      std::vector<efanna2e::Neighbor>& retset,
      const float *query,
      const float *base,
      size_t L)
{
  data_ = base;
  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    float dist = distance_->compare(data_ + dimension_ * id, query, (unsigned)dimension_);
    retset[i] = Neighbor(id, dist, true);
    // flags[id] = true;
  }
  std::sort(retset.begin(), retset.begin() + L);
}

void IndexNSG::Save(std::string filename) {
  std::ofstream out(filename, std::ios::binary | std::ios::out);
  assert(final_graph_.size() == nd_);

  out.write((char *)&width, sizeof(unsigned));
  out.write((char *)&ep_, sizeof(unsigned));
  for (unsigned i = 0; i < nd_; i++) {
    unsigned GK = (unsigned)final_graph_[i].size();
    out.write((char *)&GK, sizeof(unsigned));
    out.write((char *)final_graph_[i].data(), GK * sizeof(unsigned));
  }
  out.close();
}

int IndexNSG::NewSearchGetData(
        std::vector<efanna2e::Neighbor>& retset,
        boost::dynamic_bitset<>& flags,
        const float *query,
        const float *base,
        size_t L, 
        int k,
        unsigned& edge_table_id,
        std::vector<unsigned>& target_ids,
        std::vector<unsigned>& compare_latency,
        uint64_t qe_name)
{
  if (k >= (int)L) {
    return -1;
  }
  int temp_k = k;
  data_ = base;
  int nk = L;
  int node_read_num = 0;
  int vec_read_num = 0;
  int compare_num = 0;
  if (retset[k].flag) {
    retset[k].flag = false;
    unsigned n = retset[k].id;
    edge_table_id = n;
    
    for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
      node_read_num+=1;
      unsigned id = final_graph_[n][m];
      if (flags[id]) 
        continue;
      flags[id] = true;
      vec_read_num+=1;
      float dist = distance_->compare(query, data_ + dimension_ * id, (unsigned)dimension_);
      target_ids.push_back(id);
      if (dist >= retset[L - 1].distance) {
        compare_latency.push_back(1);
        continue;
      }
      Neighbor nn(id, dist, true);
      unsigned temp_latency = 0;
      int r = InsertIntoPool_withCompareCount(retset.data(), L, nn, temp_latency);
      compare_latency.push_back(temp_latency);
      // if (thread_zero){
      //   std::cout << "Insert_" << insert_count << " r= " << r << std::endl;
      // }
      if (r < nk) {
        nk = r;
      }
    }
    
  }
  if (nk <= k){
    k = nk;
  }
  else{
    k++;
  }
  if(qe_name == 0 && (node_read_num != 0 || vec_read_num != 0)){
    std::cout << "k: " << temp_k;
    std::cout << " node_read_num: " << node_read_num;
    std::cout << " vec_read_num: " << vec_read_num;
    std::cout << " compare_num: " << compare_num;
    std::cout << " target_ids: ";
    // for (auto &id : target_ids){
    //   cout << id << " ";
    // }
    // std::cout << " compare_latency: ";
    // for (auto &latency : compare_latency){
    //   cout << latency << " ";
    // }
  }
  return k;
}

void IndexNSG::Print_Edge_Vec(){
  int cnt = 0;
  uint64_t start_addr = (uint64_t)(&final_graph_[0]);
  uint64_t start_addr1 = (uint64_t)(&final_graph_[0][0]);
  std::cout << "delta: " << std::dec << start_addr1 - start_addr << std::endl;
  for (unsigned i = 0; i < nd_; i++) {
    std::cout << "***************" << std::endl;
    std::cout << "i = " << i << ": " << final_graph_[i].size() << " Phyaddr: " << std::dec << (uint64_t)(&final_graph_[i]) - start_addr << std::endl;
    for (unsigned j = 0; j < final_graph_[i].size(); j++) {
      
      std::cout << final_graph_[i][j] << " j = " << j << " Phy_addr: " << std::dec << (uint64_t)(&final_graph_[i][j]) - start_addr1 << std::endl; 
      cnt++;    
    }
    if (cnt > 10000)
      break;
    std::cout << std::endl;
  }
}

void IndexNSG::GetSizeAndAddr(unsigned id, int& size, uint64_t& addr){
  size = (int)(final_graph_[id].size()*(sizeof(unsigned)));
  addr = (uint64_t)(accumulate_nsg_size[id]*(sizeof(unsigned)));
}

void IndexNSG::Load(std::string filename) {
  std::ifstream in(filename, std::ios::binary);
  in.read((char *)&width, sizeof(unsigned));
  in.read((char *)&ep_, sizeof(unsigned));
  // width=100;
  unsigned cc = 0;
  while (!in.eof()) {
    unsigned k;
    in.read((char *)&k, sizeof(unsigned));
    if (in.eof())
      break;
    cc += k;
    // std::cout<< "cc: " << cc;
    // std::cout<< " k= " << k <<std::endl;
    accumulate_nsg_size.push_back(cc);
    std::vector<unsigned> tmp(k);
    in.read((char *)tmp.data(), k * sizeof(unsigned));
    final_graph_.push_back(tmp);
  }
  std::cout<<cc<<std::endl;
  std::cout<<nd_<<std::endl;
  cc /= nd_;
  // std::cout<<cc<<std::endl;
  std::cout<<accumulate_nsg_size.size()<<std::endl;
  // std::cout<<accumulate_nsg_size[3]<<std::endl;
  // std::cout<<accumulate_nsg_size[6]<<std::endl;
}
void IndexNSG::Load_nn_graph(const char *filename) {
  std::ifstream in(filename, std::ios::binary);
  unsigned k;
  in.read((char *)&k, sizeof(unsigned));
  in.seekg(0, std::ios::end);
  std::ios::pos_type ss = in.tellg();
  size_t fsize = (size_t)ss;
  size_t num = (unsigned)(fsize / (k + 1) / 4);
  in.seekg(0, std::ios::beg);

  final_graph_.resize(num);
  final_graph_.reserve(num);
  unsigned kk = (k + 3) / 4 * 4;
  for (size_t i = 0; i < num; i++) {
    in.seekg(4, std::ios::cur);
    final_graph_[i].resize(k);
    final_graph_[i].reserve(kk);
    in.read((char *)final_graph_[i].data(), k * sizeof(unsigned));
  }
  in.close();
}

void IndexNSG::get_neighbors(const float *query, const Parameters &parameter,
                             std::vector<Neighbor> &retset,
                             std::vector<Neighbor> &fullset) {
  unsigned L = parameter.Get<unsigned>("L");

  retset.resize(L + 1);
  std::vector<unsigned> init_ids(L);
  // initializer_->Search(query, nullptr, L, parameter, init_ids.data());

  boost::dynamic_bitset<> flags{nd_, 0};
  L = 0;
  for (unsigned i = 0; i < init_ids.size() && i < final_graph_[ep_].size();
       i++) {
    init_ids[i] = final_graph_[ep_][i];
    flags[init_ids[i]] = true;
    L++;
  }
  while (L < init_ids.size()) {
    unsigned id = rand() % nd_;
    if (flags[id])
      continue;
    init_ids[L] = id;
    L++;
    flags[id] = true;
  }

  L = 0;
  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    if (id >= nd_)
      continue;
    // std::cout<<id<<std::endl;
    float dist = distance_->compare(data_ + dimension_ * (size_t)id, query,
                                    (unsigned)dimension_);
    retset[i] = Neighbor(id, dist, true);
    // flags[id] = 1;
    L++;
  }

  std::sort(retset.begin(), retset.begin() + L);
  int k = 0;
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;

      for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
        unsigned id = final_graph_[n][m];
        if (flags[id])
          continue;
        flags[id] = 1;

        float dist = distance_->compare(query, data_ + dimension_ * (size_t)id,
                                        (unsigned)dimension_);
        Neighbor nn(id, dist, true);
        fullset.push_back(nn);
        if (dist >= retset[L - 1].distance)
          continue;
        int r = InsertIntoPool(retset.data(), L, nn);

        if (L + 1 < retset.size())
          ++L;
        if (r < nk)
          nk = r;
      }
    }
    if (nk <= k)
      k = nk;
    else
      ++k;
  }
}

void IndexNSG::get_neighbors(const float *query, const Parameters &parameter,
                             boost::dynamic_bitset<> &flags,
                             std::vector<Neighbor> &retset,
                             std::vector<Neighbor> &fullset) {
  unsigned L = parameter.Get<unsigned>("L");

  retset.resize(L + 1);
  std::vector<unsigned> init_ids(L);
  // initializer_->Search(query, nullptr, L, parameter, init_ids.data());

  L = 0;
  for (unsigned i = 0; i < init_ids.size() && i < final_graph_[ep_].size();
       i++) {
    init_ids[i] = final_graph_[ep_][i];
    flags[init_ids[i]] = true;
    L++;
  }
  while (L < init_ids.size()) {
    unsigned id = rand() % nd_;
    if (flags[id])
      continue;
    init_ids[L] = id;
    L++;
    flags[id] = true;
  }

  L = 0;
  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    if (id >= nd_)
      continue;
    // std::cout<<id<<std::endl;
    float dist = distance_->compare(data_ + dimension_ * (size_t)id, query,
                                    (unsigned)dimension_);
    retset[i] = Neighbor(id, dist, true);
    fullset.push_back(retset[i]);
    // flags[id] = 1;
    L++;
  }

  std::sort(retset.begin(), retset.begin() + L);
  int k = 0;
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;

      for (unsigned m = 0; m < final_graph_[n].size(); ++m) {
        unsigned id = final_graph_[n][m];
        if (flags[id])
          continue;
        flags[id] = 1;

        float dist = distance_->compare(query, data_ + dimension_ * (size_t)id,
                                        (unsigned)dimension_);
        Neighbor nn(id, dist, true);
        fullset.push_back(nn);
        if (dist >= retset[L - 1].distance)
          continue;
        int r = InsertIntoPool(retset.data(), L, nn);

        if (L + 1 < retset.size())
          ++L;
        if (r < nk)
          nk = r;
      }
    }
    if (nk <= k)
      k = nk;
    else
      ++k;
  }
}

void IndexNSG::init_graph(const Parameters &parameters) {
  float *center = new float[dimension_];
  for (unsigned j = 0; j < dimension_; j++)
    center[j] = 0;
  for (unsigned i = 0; i < nd_; i++) {
    for (unsigned j = 0; j < dimension_; j++) {
      center[j] += data_[i * dimension_ + j];
    }
  }
  for (unsigned j = 0; j < dimension_; j++) {
    center[j] /= nd_;
  }
  std::vector<Neighbor> tmp, pool;
  ep_ = rand() % nd_; // random initialize navigating point
  get_neighbors(center, parameters, tmp, pool);
  ep_ = tmp[0].id;
}

void IndexNSG::sync_prune(unsigned q, std::vector<Neighbor> &pool,
                          const Parameters &parameter,
                          boost::dynamic_bitset<> &flags,
                          SimpleNeighbor *cut_graph_) {
  unsigned range = parameter.Get<unsigned>("R");
  unsigned maxc = parameter.Get<unsigned>("C");
  width = range;
  unsigned start = 0;

  for (unsigned nn = 0; nn < final_graph_[q].size(); nn++) {
    unsigned id = final_graph_[q][nn];
    if (flags[id])
      continue;
    float dist = distance_->compare(data_ + dimension_ * (size_t)q,
                                    data_ + dimension_ * (size_t)id,
                                    (unsigned)dimension_);
    pool.push_back(Neighbor(id, dist, true));
  }

  std::sort(pool.begin(), pool.end());
  std::vector<Neighbor> result;
  if (pool[start].id == q)
    start++;
  result.push_back(pool[start]);

  while (result.size() < range && (++start) < pool.size() && start < maxc) {
    auto &p = pool[start];
    bool occlude = false;
    for (unsigned t = 0; t < result.size(); t++) {
      if (p.id == result[t].id) {
        occlude = true;
        break;
      }
      float djk = distance_->compare(data_ + dimension_ * (size_t)result[t].id,
                                     data_ + dimension_ * (size_t)p.id,
                                     (unsigned)dimension_);
      if (djk < p.distance /* dik */) {
        occlude = true;
        break;
      }
    }
    if (!occlude)
      result.push_back(p);
  }

  SimpleNeighbor *des_pool = cut_graph_ + (size_t)q * (size_t)range;
  for (size_t t = 0; t < result.size(); t++) {
    des_pool[t].id = result[t].id;
    des_pool[t].distance = result[t].distance;
  }
  if (result.size() < range) {
    des_pool[result.size()].distance = -1;
  }
}

void IndexNSG::InterInsert(unsigned n, unsigned range,
                           std::vector<std::mutex> &locks,
                           SimpleNeighbor *cut_graph_) {
  SimpleNeighbor *src_pool = cut_graph_ + (size_t)n * (size_t)range;
  for (size_t i = 0; i < range; i++) {
    if (src_pool[i].distance == -1)
      break;

    SimpleNeighbor sn(n, src_pool[i].distance);
    size_t des = src_pool[i].id;
    SimpleNeighbor *des_pool = cut_graph_ + des * (size_t)range;

    std::vector<SimpleNeighbor> temp_pool;
    int dup = 0;
    {
      LockGuard guard(locks[des]);
      for (size_t j = 0; j < range; j++) {
        if (des_pool[j].distance == -1)
          break;
        if (n == des_pool[j].id) {
          dup = 1;
          break;
        }
        temp_pool.push_back(des_pool[j]);
      }
    }
    if (dup)
      continue;

    temp_pool.push_back(sn);
    if (temp_pool.size() > range) {
      std::vector<SimpleNeighbor> result;
      unsigned start = 0;
      std::sort(temp_pool.begin(), temp_pool.end());
      result.push_back(temp_pool[start]);
      while (result.size() < range && (++start) < temp_pool.size()) {
        auto &p = temp_pool[start];
        bool occlude = false;
        for (unsigned t = 0; t < result.size(); t++) {
          if (p.id == result[t].id) {
            occlude = true;
            break;
          }
          float djk = distance_->compare(
              data_ + dimension_ * (size_t)result[t].id,
              data_ + dimension_ * (size_t)p.id, (unsigned)dimension_);
          if (djk < p.distance /* dik */) {
            occlude = true;
            break;
          }
        }
        if (!occlude)
          result.push_back(p);
      }
      {
        LockGuard guard(locks[des]);
        for (unsigned t = 0; t < result.size(); t++) {
          des_pool[t] = result[t];
        }
        if (result.size() < range) {
          des_pool[result.size()].distance = -1;
        }
      }
    } else {
      LockGuard guard(locks[des]);
      for (unsigned t = 0; t < range; t++) {
        if (des_pool[t].distance == -1) {
          des_pool[t] = sn;
          if (t + 1 < range)
            des_pool[t + 1].distance = -1;
          break;
        }
      }
    }
  }
}

void IndexNSG::Link(const Parameters &parameters, SimpleNeighbor *cut_graph_) {
  /*
  std::cout << " graph link" << std::endl;
  unsigned progress=0;
  unsigned percent = 100;
  unsigned step_size = nd_/percent;
  std::mutex progress_lock;
  */
  unsigned range = parameters.Get<unsigned>("R");
  std::vector<std::mutex> locks(nd_);

#pragma omp parallel
  {
    // unsigned cnt = 0;
    std::vector<Neighbor> pool, tmp;
    boost::dynamic_bitset<> flags{nd_, 0};
#pragma omp for schedule(dynamic, 100)
    for (unsigned n = 0; n < nd_; ++n) {
      pool.clear();
      tmp.clear();
      flags.reset();
      get_neighbors(data_ + dimension_ * n, parameters, flags, tmp, pool);
      sync_prune(n, pool, parameters, flags, cut_graph_);
      /*
    cnt++;
    if(cnt % step_size == 0){
      LockGuard g(progress_lock);
      std::cout<<progress++ <<"/"<< percent << " completed" << std::endl;
      }
      */
    }
  }

#pragma omp for schedule(dynamic, 100)
  for (unsigned n = 0; n < nd_; ++n) {
    InterInsert(n, range, locks, cut_graph_);
  }
}
// TODO : n is not used?
void IndexNSG::Build(size_t /*n*/, const float *data,
                     const Parameters &parameters) {
  std::string nn_graph_path = parameters.Get<std::string>("nn_graph_path");
  unsigned range = parameters.Get<unsigned>("R");
  Load_nn_graph(nn_graph_path.c_str());
  data_ = data;
  init_graph(parameters);
  SimpleNeighbor *cut_graph_ = new SimpleNeighbor[nd_ * (size_t)range];
  Link(parameters, cut_graph_);
  final_graph_.resize(nd_);

  for (size_t i = 0; i < nd_; i++) {
    SimpleNeighbor *pool = cut_graph_ + i * (size_t)range;
    unsigned pool_size = 0;
    for (unsigned j = 0; j < range; j++) {
      if (pool[j].distance == -1)
        break;
      pool_size = j;
    }
    pool_size++;
    final_graph_[i].resize(pool_size);
    for (unsigned j = 0; j < pool_size; j++) {
      final_graph_[i][j] = pool[j].id;
    }
  }

  tree_grow(parameters);

  unsigned max = 0, min = 1e6, avg = 0;
  for (size_t i = 0; i < nd_; i++) {
    auto size = final_graph_[i].size();
    max = max < size ? size : max;
    min = min > size ? size : min;
    avg += size;
  }
  avg /= 1.0 * nd_;
  printf("Degree Statistics: Max = %d, Min = %d, Avg = %d\n", max, min, avg);

  has_built = true;
}

// TODO empty function?
void IndexNSG::Insert_node(unsigned ) {}

bool IndexNSG::Exist_node(unsigned ) {
  //TODO implement
  return false;
}

void IndexNSG::SearchWithOptGraph(const float *query, size_t K,
                                  const Parameters &parameters,
                                  unsigned *indices) {
  unsigned L = parameters.Get<unsigned>("L_search");
  DistanceFastL2 *dist_fast = (DistanceFastL2 *)distance_;

  std::vector<Neighbor> retset(L + 1);
  std::vector<unsigned> init_ids(L);
  // std::mt19937 rng(rand());
  // GenRandom(rng, init_ids.data(), L, (unsigned) nd_);

  boost::dynamic_bitset<> flags{nd_, 0};
  unsigned tmp_l = 0;
  unsigned *neighbors = (unsigned *)(opt_graph_ + node_size * ep_ + data_len);
  unsigned MaxM_ep = *neighbors;
  neighbors++;

  for (; tmp_l < L && tmp_l < MaxM_ep; tmp_l++) {
    init_ids[tmp_l] = neighbors[tmp_l];
    flags[init_ids[tmp_l]] = true;
  }

  while (tmp_l < L) {
    unsigned id = rand() % nd_;
    if (flags[id])
      continue;
    flags[id] = true;
    init_ids[tmp_l] = id;
    tmp_l++;
  }

  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    if (id >= nd_)
      continue;
    _mm_prefetch(opt_graph_ + node_size * id, _MM_HINT_T0);
  }
  L = 0;
  for (unsigned i = 0; i < init_ids.size(); i++) {
    unsigned id = init_ids[i];
    if (id >= nd_)
      continue;
    float *x = (float *)(opt_graph_ + node_size * id);
    float norm_x = *x;
    x++;
    float dist = dist_fast->compare(x, query, norm_x, (unsigned)dimension_);
    retset[i] = Neighbor(id, dist, true);
    flags[id] = true;
    L++;
  }
  // std::cout<<L<<std::endl;

  std::sort(retset.begin(), retset.begin() + L);
  int k = 0;
  while (k < (int)L) {
    int nk = L;

    if (retset[k].flag) {
      retset[k].flag = false;
      unsigned n = retset[k].id;

      _mm_prefetch(opt_graph_ + node_size * n + data_len, _MM_HINT_T0);
      unsigned *neighbors = (unsigned *)(opt_graph_ + node_size * n + data_len);
      unsigned MaxM = *neighbors;
      neighbors++;
      for (unsigned m = 0; m < MaxM; ++m)
        _mm_prefetch(opt_graph_ + node_size * neighbors[m], _MM_HINT_T0);
      for (unsigned m = 0; m < MaxM; ++m) {
        unsigned id = neighbors[m];
        if (flags[id])
          continue;
        flags[id] = 1;
        float *data = (float *)(opt_graph_ + node_size * id);
        float norm = *data;
        data++;
        float dist =
            dist_fast->compare(query, data, norm, (unsigned)dimension_);
        if (dist >= retset[L - 1].distance)
          continue;
        Neighbor nn(id, dist, true);
        int r = InsertIntoPool(retset.data(), L, nn);

        // if(L+1 < retset.size()) ++L;
        if (r < nk)
          nk = r;
      }
    }
    if (nk <= k)
      k = nk;
    else
      ++k;
  }
  for (size_t i = 0; i < K; i++) {
    indices[i] = retset[i].id;
  }
}

void IndexNSG::OptimizeGraph(float *data) { // use after build or load

  data_ = data;
  data_len = (dimension_ + 1) * sizeof(float);
  neighbor_len = (width + 1) * sizeof(unsigned);
  node_size = data_len + neighbor_len;
  opt_graph_ = (char *)malloc(node_size * nd_);
  DistanceFastL2 *dist_fast = (DistanceFastL2 *)distance_;
  for (unsigned i = 0; i < nd_; i++) {
    char *cur_node_offset = opt_graph_ + i * node_size;
    float cur_norm = dist_fast->norm(data_ + i * dimension_, dimension_);
    std::memcpy(cur_node_offset, &cur_norm, sizeof(float));
    std::memcpy(cur_node_offset + sizeof(float), data_ + i * dimension_,
                data_len - sizeof(float));

    cur_node_offset += data_len;
    unsigned k = final_graph_[i].size();
    std::memcpy(cur_node_offset, &k, sizeof(unsigned));
    std::memcpy(cur_node_offset + sizeof(unsigned), final_graph_[i].data(),
                k * sizeof(unsigned));
    std::vector<unsigned>().swap(final_graph_[i]);
  }
  CompactGraph().swap(final_graph_);
}

void IndexNSG::DFS(boost::dynamic_bitset<> &flag, unsigned root,
                   unsigned &cnt) {
  unsigned tmp = root;
  std::stack<unsigned> s;
  s.push(root);
  if (!flag[root])
    cnt++;
  flag[root] = true;
  while (!s.empty()) {
    unsigned next = nd_ + 1;
    for (unsigned i = 0; i < final_graph_[tmp].size(); i++) {
      if (flag[final_graph_[tmp][i]] == false) {
        next = final_graph_[tmp][i];
        break;
      }
    }
    // std::cout << next <<":"<<cnt <<":"<<tmp <<":"<<s.size()<< '\n';
    if (next == (nd_ + 1)) {
      s.pop();
      if (s.empty())
        break;
      tmp = s.top();
      continue;
    }
    tmp = next;
    flag[tmp] = true;
    s.push(tmp);
    cnt++;
  }
}

void IndexNSG::findroot(boost::dynamic_bitset<> &flag, unsigned &root,
                        const Parameters &parameter) {
  unsigned id = nd_;
  for (unsigned i = 0; i < nd_; i++) {
    if (flag[i] == false) {
      id = i;
      break;
    }
  }

  if (id == nd_)
    return; // No Unlinked Node

  std::vector<Neighbor> tmp, pool;
  get_neighbors(data_ + dimension_ * id, parameter, tmp, pool);
  std::sort(pool.begin(), pool.end());

  unsigned found = 0;
  for (unsigned i = 0; i < pool.size(); i++) {
    if (flag[pool[i].id]) {
      // std::cout << pool[i].id << '\n';
      root = pool[i].id;
      found = 1;
      break;
    }
  }
  if (found == 0) {
    while (true) {
      unsigned rid = rand() % nd_;
      if (flag[rid]) {
        root = rid;
        break;
      }
    }
  }
  final_graph_[root].push_back(id);
}
void IndexNSG::tree_grow(const Parameters &parameter) {
  unsigned root = ep_;
  boost::dynamic_bitset<> flags{nd_, 0};
  unsigned unlinked_cnt = 0;
  while (unlinked_cnt < nd_) {
    DFS(flags, root, unlinked_cnt);
    // std::cout << unlinked_cnt << '\n';
    if (unlinked_cnt >= nd_)
      break;
    findroot(flags, root, parameter);
    // std::cout << "new root"<<":"<<root << '\n';
  }
  for (size_t i = 0; i < nd_; ++i) {
    if (final_graph_[i].size() > width) {
      width = final_graph_[i].size();
    }
  }
}
} // namespace efanna2e
