# the near memory nsg simulator
1. start up
- git clone --recursive https://github.com/shenjiangqiu/near_mem_nsg.git 
- component base: read component.h for more detail
- ramulator : read tests/ramulator_test.cc
- task_queue: use the tasks queue to implement pipeline: read tests/task_queue_test.cc and controller.h/cc
- other utilities:
    - log: log_test.cc
    - ...
2. configure vscode:
- install the following plugin:
    1. cmake
    2. c++ TestMate
- then select dev toolkit from bottom bar
-  then build the project from left bar
-  then test the project from left bar

3. things need todo
- generate trace from nsg and push to controller
- generate task from controller and push to pe
- get the cycle funciton in pe from task
