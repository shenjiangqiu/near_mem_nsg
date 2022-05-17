#!/bin/bash
if [ "${1}" == "sift1M" ]; then
  if [ ! -f "sift1M.nsg" ]; then
    echo "Converting sift_200nn.graph kNN graph to sift.nsg"
    if [ -f "sift_200nn.graph" ]; then
      ./test_nsg_index sift1M/sift_base.fvecs sift_200nn.graph 40 50 500 sift1M.nsg > sift1M.index.log
    else
      echo "ERROR: sift_200nn.graph does not exist"
      exit 1
    fi
  fi
  echo "Perform kNN searching using NSG index"
  ./test_nsg_optimized_search sift1M/sift_base.fvecs sift1M/sift_query.fvecs sift1M.nsg 200 200 . > sift1M.search.log
elif [ "${1}" == "gist1M" ]; then
  if [ ! -f "gist1M.nsg" ]; then
    echo "Converting gist_400nn.graph kNN graph to gist.nsg"
    if [ -f "gist_400nn.graph" ]; then
      ./test_nsg_index gist1M/gist_base.fvecs gist_400nn.graph 60 70 500 gist1M.nsg > gist1M.index.log
    else
      echo "ERROR: gist_400nn.graph does not exist"
      exit 1
    fi
  fi
  echo "Perform kNN searching using NSG index"
  ./test_nsg_optimized_search gist1M/gist_base.fvecs gist1M/gist_query.fvecs gist1M.nsg 400 400 . > gist1M.search.log
elif [ "${1}" == "all" ]; then
  if [ ! -f "sift1M.nsg" ]; then
    echo "Converting sift_200nn.graph kNN graph to sift.nsg"
    if [ -f "sift_200nn.graph" ]; then
      ./test_nsg_index sift1M/sift_base.fvecs sift_200nn.graph 40 50 500 sift1M.nsg > sift1M.index.log
    else
      echo "ERROR: sift_200nn.graph does not exist"
      exit 1
    fi
  fi
  echo "Perform kNN searching using NSG index"
  ./test_nsg_optimized_search sift1M/sift_base.fvecs sift1M/sift_query.fvecs sift1M.nsg 200 200 . > sift1M.search.log

  if [ ! -f "gist1M.nsg" ]; then
    echo "Converting gist_400nn.graph kNN graph to gist.nsg"
    if [ -f "gist_400nn.graph" ]; then
      ./test_nsg_index gist1M/gist_base.fvecs gist_400nn.graph 60 70 500 gist1M.nsg > gist1M.index.log
    else
      echo "ERROR: gist_400nn.graph does not exist"
      exit 1
    fi
  fi
  echo "Perform kNN searching using NSG index"
  ./test_nsg_optimized_search gist1M/gist_base.fvecs gist1M/gist_query.fvecs gist1M.nsg 400 400 . > gist1M.search.log
else
  echo "Please use either 'sift' or 'gist' as an argument"
fi
