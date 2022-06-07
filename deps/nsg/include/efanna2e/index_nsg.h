#ifndef EFANNA2E_INDEX_NSG_H
#define EFANNA2E_INDEX_NSG_H

#include "util.h"
#include "parameters.h"
#include "neighbor.h"
#include "index.h"
#include "PrimitiveBloomFilter.h"
#include <cassert>
#include <unordered_map>
#include <string>
#include <sstream>
#include <boost/dynamic_bitset.hpp>
#include <stack>
#include <omp.h>
#include <bitset>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
// using namespace std;
// using namespace std::chrono;
// system_clock::time_point now = system_clock::now();
namespace efanna2e {

class IndexNSG : public Index {
public:
    explicit IndexNSG(const size_t dimension, const size_t n, Metric m, Index *initializer);


    virtual ~IndexNSG();

    virtual void Save(std::string filename)override;
    virtual void Load(std::string filename)override;


    virtual void Build(size_t n, const float *data, const Parameters &parameters) override;

    virtual void Search(
        const float *query,
        const float *x,
        size_t k,
        const Parameters &parameters,
        unsigned *indices,
        bool thread_zero) override;
    void SearchWithOptGraph(
        const float *query,
        size_t K,
        const Parameters &parameters,
        unsigned *indices);
    void OptimizeGraph(float* data);
    void Insert_node(unsigned node_id);
    bool Exist_node(unsigned node_id);
    //near_mem_nsg
    void PreProcess(
        std::vector<unsigned>& init_ids, 
        PrimitiveBloomFilter<unsigned,80000>& BF, 
        boost::dynamic_bitset<>& flags,
        size_t L);
    void NewSearch(
        std::vector<unsigned>& init_ids,
        PrimitiveBloomFilter<unsigned,80000>& BF,
        boost::dynamic_bitset<>& flags,
        const float *query,
        const float *base,
        size_t K,
        size_t L,
        unsigned *indices,
        bool thread_zero);
    void NewSearchInit(
        std::vector<unsigned>& init_ids,
        std::vector<efanna2e::Neighbor>& retset,
        const float *query,
        const float *base,
        size_t L);
    int NewSearchGetData(
        std::vector<efanna2e::Neighbor>& retset,
        boost::dynamic_bitset<>& flags,
        const float *query,
        const float *base,
        size_t L, 
        int k,
        unsigned& edge_table_id,
        std::vector<unsigned>& target_ids);

protected:
    typedef std::vector<std::vector<unsigned > > CompactGraph;
    typedef std::vector<SimpleNeighbors > LockGraph;
    typedef std::vector<nhood> KNNGraph;

    CompactGraph final_graph_;

    Index *initializer_;
    void init_graph(const Parameters &parameters);
    void get_neighbors(
        const float *query,
        const Parameters &parameter,
        std::vector<Neighbor> &retset,
        std::vector<Neighbor> &fullset);
    void get_neighbors(
        const float *query,
        const Parameters &parameter,
        boost::dynamic_bitset<>& flags,
        std::vector<Neighbor> &retset,
        std::vector<Neighbor> &fullset);
    //void add_cnn(unsigned des, Neighbor p, unsigned range, LockGraph& cut_graph_);
    void InterInsert(unsigned n, unsigned range, std::vector<std::mutex>& locks, SimpleNeighbor* cut_graph_);
    void sync_prune(unsigned q, std::vector<Neighbor>& pool, const Parameters &parameter, boost::dynamic_bitset<>& flags, SimpleNeighbor* cut_graph_);
    void Link(const Parameters &parameters, SimpleNeighbor* cut_graph_);
    void Load_nn_graph(const char *filename);
    void tree_grow(const Parameters &parameter);
    void DFS(boost::dynamic_bitset<> &flag, unsigned root, unsigned &cnt);
    void findroot(boost::dynamic_bitset<> &flag, unsigned &root, const Parameters &parameter);


private:
    unsigned width;
    unsigned ep_;
    std::vector<std::mutex> locks;
    char* opt_graph_;
    size_t node_size;
    size_t data_len;
    size_t neighbor_len;
    KNNGraph nnd_graph;
};
}

#endif //EFANNA2E_INDEX_NSG_H
