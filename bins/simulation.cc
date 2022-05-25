#include <component.h>
#include <controller.h>
#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <qe.h>
#include <struct.h>
#include <task_queue.h>

using namespace near_mem;
int main() {
  plog::init(plog::debug, "test.log", 100 * 1024, 3);
  unsigned num_pes = 100;
  std::vector<Sender<NsgTask>> task_senders;
  std::vector<Receiver<NsgTask>> task_receivers;
  for (auto i = 0u; i < num_pes; ++i) {
    auto [sender, receiver] = make_task_queue<NsgTask>(4);
    task_senders.push_back(sender);
    task_receivers.push_back(receiver);
  }
  auto [mem_sender, mem_receiver] = make_task_queue<uint64_t>(4);
  auto [mem_ret_sender, mem_ret_receiver] = make_task_queue<uint64_t>(4);
  uint64_t current_cycle = 0;
  auto controller = Controller(current_cycle, std::move(task_senders),
                               mem_sender, mem_ret_receiver, 100, 1,
                               efanna2e::Metric::L2, nullptr, nullptr, 10, 100);

  std::vector<Qe> qes;
  for (auto i = 0u; i < num_pes; ++i) {
    std::vector<Sender<QeTask>> qe_senders;
    std::vector<Receiver<QeTask>> qe_receivers;
    for (auto j = 0u; j < num_pes; ++j) {
      auto [sender, receiver] = make_task_queue<QeTask>(4);
      qe_senders.push_back(sender);
      qe_receivers.push_back(receiver);
    }

    qes.push_back(Qe(current_cycle, qe_senders, task_receivers[i]));
  }

  std::vector<Component *> components;
  components.push_back(&controller);
  for (auto &qe : qes) {
    components.push_back(&qe);
  }
  while (true) {
    for (auto component : components) {
      component->cycle();
      current_cycle++;
    }
    bool all_end = true;

    for (auto component : components) {
      if (!component->all_end()) {
        all_end = false;
      }
    }
    if (all_end) {
      break;
    }
  }
}
