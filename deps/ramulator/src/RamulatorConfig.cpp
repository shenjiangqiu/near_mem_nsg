#include "RamulatorConfig.h"

using namespace std;
using namespace ramulator;

Config::Config(const std::string& fname) {
  parse(fname);
}

void Config::parse(const string& fname)
{
    ifstream file(fname);
    assert(file.good() && "Bad config file");
    string line;
    while (getline(file, line)) {
        char delim[] = " \t=";
        vector<string> tokens;

        while (true) {
            size_t start = line.find_first_not_of(delim);
            if (start == string::npos) 
                break;

            size_t end = line.find_first_of(delim, start);
            if (end == string::npos) {
                tokens.push_back(line.substr(start));
                break;
            }

            tokens.push_back(line.substr(start, end - start));
            line = line.substr(end);
        }

        // empty line
        if (!tokens.size())
            continue;

        // comment line
        if (tokens[0][0] == '#')
            continue;

        // parameter line
        assert(tokens.size() == 2 && "Only allow two tokens in one line");

        options[tokens[0]] = tokens[1];

        if (tokens[0] == "channels") {
          channels = atoi(tokens[1].c_str());
        } else if (tokens[0] == "ranks") {
          ranks = atoi(tokens[1].c_str());
        } else if (tokens[0] == "subarrays") {
          subarrays = atoi(tokens[1].c_str());
        } else if (tokens[0] == "cpu_tick") {
          cpu_tick = atoi(tokens[1].c_str());
        } else if (tokens[0] == "mem_tick") {
          mem_tick = atoi(tokens[1].c_str());
        } else if (tokens[0] == "tCK") {
          tCK = atoi(tokens[1].c_str());
        } else if (tokens[0] == "expected_limit_insts") {
          expected_limit_insts = atoi(tokens[1].c_str());
        } else if (tokens[0] == "warmup_insts") {
          warmup_insts = atoi(tokens[1].c_str());
        }else if (tokens[0] == "addr_mapping") { //Yue
            //std::cout<<" addr_mapping string "<<tokens[1];
            if (tokens[1] == "ChRaBaRoCo")
              addr_mapping = 0;
            else if (tokens[1] == "RoBaRaCoCh")
              addr_mapping = 1;
            else if (tokens[1] == "CoRoBaRaCh")
              addr_mapping = 2;
            else if (tokens[1] == "RoCoBaRaCh")
              addr_mapping = 3;  
            else
              assert(0);
            //std::cout<<"  "<<addr_mapping <<std::endl;
        }
        
    }
    file.close();
}


