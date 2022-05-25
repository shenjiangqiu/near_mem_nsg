#ifndef DC_H
#define DC_H

#include <component.h>
#include <struct.h>
#include <task_queue.h>
#include <fmt/format.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>

namespace near_mem {
class Distance_Computer : public Component {
    using DistanceReceiver = Receiver<DistanceTask>;
    using DistanceSender = Sender<DistanceTask>;
    
public:
    Distance_Computer(
        uint64_t &current_cycle,
        uint64_t dc_name,
        DistanceReceiver distance_receiver,
        DistanceSender return_distance_sender
        ) :
    Component(current_cycle),
    dc_name(dc_name),
    distance_receiver(distance_receiver),
    return_distance_sender(return_distance_sender) {}

    bool do_cycle() override;
    bool all_end() const override {
        auto end = !working();
        return end;
    }
    std::string get_internal_size() const override {
        return std::string("");
    }; // TODO add the internal status for the controller for debug
    std::string get_line_trace() const override {
        return std::string("");
    }; //
    bool working() const {
        return remaining_cycle > 0;
    }

private:
    uint64_t dc_name;
    DistanceReceiver distance_receiver;
    DistanceSender return_distance_sender;

    unsigned query_id = 0;
    uint64_t occupy_qe_name = 0;

    unsigned remaining_cycle = 0;

}; 
}// namespace near_mem

#endif // DC_H