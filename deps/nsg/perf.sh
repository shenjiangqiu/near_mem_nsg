#!/bin/bash
echo "Perform NSG_$1 profiling..."
if [ $# == 1 ]; then
  perf stat -e LLC-load-misses,LLC-loads,LLC-store-misses,LLC-stores,L1-dcache-loads,L1-dcache-stores,mem_inst_retired.all_loads,mem_inst_retired.all_stores,mem_load_retired.l3_hit,mem_load_retired.l3_miss,instructions -o $1.perf ./run.sh $1
else
  echo "Please use either 'sift' or 'gist' as an argument"
fi
