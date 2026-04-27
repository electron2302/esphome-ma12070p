#include "mute_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ma12070p {

static const char *const TAG = "ma12070p.switch";

void MuteSwitch::setup() {
  optional<bool> initial_state = this->get_initial_state_with_restore_mode();
  bool setup_state = initial_state.has_value() ? initial_state.value() : true;
  this->write_state(setup_state);
}

void MuteSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "Ma12070 Switch:");
  LOG_SWITCH("  ", "Mute", this);
}

void MuteSwitch::write_state(bool state) {
  this->publish_state(state);
  if (state)
    this->parent_->set_mute_on();
  else
    this->parent_->set_mute_off();
}

}  // namespace ma12070p
}  // namespace esphome
