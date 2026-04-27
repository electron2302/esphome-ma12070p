#pragma once

#include "../ma12070p.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace ma12070p {

class EnableDacSwitch : public switch_::Switch, public Component, public Parented<Ma12070Component> {
public:
public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

protected:
  void write_state(bool state) override;
};

}  // namespace ma12070p
}  // namespace esphome