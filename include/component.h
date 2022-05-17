#ifndef COMPONET_H
#define COMPONET_H
#include <iostream>
#include <string>
namespace near_mem {
class componet {
public:
  virtual std::string get_internal_size() const { return string(""); };
  // virtual double get_busy_percent() const = 0;
  virtual std::string get_line_trace() const { return string(""); };
  componet(uint64_t &tcurrent_cycle) : current_cycle(tcurrent_cycle) {}
  uint64_t &current_cycle;
  // static uint64_t current_cycle;
  // bool busy;
  virtual ~componet() = default;
  bool cycle() {
    auto result = do_cycle();
    if (result) {
      busy++;
    } else {
      idle++;
    }
    return result;
  }
  double get_busy_percent() const { return double(busy) / double(busy + idle); }

protected:
  virtual bool do_cycle() = 0;

  uint64_t busy = 0;
  uint64_t idle = 0;
};
} // namespace near_mem
#endif