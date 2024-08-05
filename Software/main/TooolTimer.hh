#ifndef TOOOLTIMER_HH_
#define TOOOLTIMER_HH_

class Button {
 public:
  Button(const uint8_t k_button_no_pin,
         const uint8_t k_button_nc_pin,
         const uint16_t k_button_mode = INPUT_PULLUP,
         const unsigned long k_debounce_delay_ms = 30)
      : k_button_no_pin_(k_button_no_pin),
        k_button_nc_pin_(k_button_nc_pin),
        k_button_mode_(k_button_mode),
        k_debounce_delay_ms_(k_debounce_delay_ms),
        last_button_state_(0),
        last_debounce_time_ms_(0) {}

  void Initialize() {}

  uint8_t IsPressed(const uint16_t iostates) {

    uint8_t no_closed = (iostates >> k_button_no_pin_) & 1u;
    uint8_t nc_closed = (iostates >> k_button_nc_pin_) & 1u;
    
    if (k_button_mode_ == INPUT_PULLUP) {
      nc_closed = !nc_closed;
      no_closed = !no_closed;
    }

    uint8_t reading = no_closed && !nc_closed;

    if (reading != last_button_state_) {
      last_debounce_time_ms_ = millis();
    }

    if ((millis() - last_debounce_time_ms_) > k_debounce_delay_ms_) {
      is_new_press_ = reading && (button_state_ != reading);
      button_state_ = reading;
    }

    last_button_state_ = reading;
    return button_state_;
  }

  uint8_t IsNewPress(const uint16_t iostates) {
    uint8_t orig_state = is_new_press_;
    is_new_press_ = false;
    return orig_state;
  }

  virtual ~Button() {}

 private:
  const uint8_t k_button_no_pin_;
  const uint8_t k_button_nc_pin_;
  const uint8_t k_button_mode_;
  const unsigned long k_debounce_delay_ms_;  // the debounce time; increase if
                                             // the output flickers
  uint8_t button_state_;       // the current reading from the input pin
  uint8_t last_button_state_;  // the previous reading from the input pin
  uint8_t is_new_press_ = false;
  unsigned long
      last_debounce_time_ms_;  // the last time the output pin was toggled
};

enum Modes {initial, starting, countup, paused, complete};

struct LockState {
  Modes lock_mode;
  
  uint8_t is_on;
  uint8_t is_open;
  uint8_t is_failed;
  uint8_t order_opened;
  uint8_t display_time;
  uint32_t lock_time;
  
} __attribute__((packed));

class Timer {
 public:
  Timer() {}

  uint8_t Initialize() {
    last_read_time_ = millis();
    timer_mode_ = initial;
    seq_opens_ = 0;
    return 0;
  }

  LockState* Step(uint16_t lock_switches, uint8_t new_mode_press, uint8_t new_play_pause_press, uint8_t new_reset_press) {
    uint32_t current_read_time = millis();
    uint32_t time_difference = current_read_time - last_read_time_;
    last_read_time_ = current_read_time;
    SetLockSwitchStates(lock_switches);
    
    if (initial == timer_mode_) {
      if (new_play_pause_press) {
        uint8_t at_least_one_ready = false;
        uint8_t no_errors_or_opened = true;
        for (int i = 0; i <8; i++) {
          if (lock_states_[i].is_on && !lock_states_[i].is_open && !lock_states_[i].is_failed){
            at_least_one_ready = true;
            lock_states_[i].lock_mode = starting;
            lock_states_[i].lock_time = 11000u;
          } else {
            lock_states_[i].lock_mode = initial;
          }
          if (lock_states_[i].is_on && (lock_states_[i].is_open || lock_states_[i].is_failed)){
            no_errors_or_opened = false;
          }
        }
        if (at_least_one_ready && no_errors_or_opened) {
          timer_mode_ = starting;  
        }
      }
      
      if (new_reset_press) {
        ResetToInitial();
      }
    } else if (starting == timer_mode_) {
      uint8_t any_starting = false;
      for (int i = 0; i < 8; i++) {
        if (starting == lock_states_[i].lock_mode) {
          if (time_difference >= lock_states_[i].lock_time) {
            lock_states_[i].lock_time = 0;
          } else {
            lock_states_[i].lock_time = lock_states_[i].lock_time - time_difference;
          }
          if (0 == lock_states_[i].lock_time) {
            lock_states_[i].lock_mode = countup;
          }
        }
        any_starting = any_starting || (starting == lock_states_[i].lock_mode);
      }
    
      if (!any_starting) {
        timer_mode_ = countup;
      }
      
      if (new_reset_press) {
        ResetToInitial();
      }
    } else if (countup == timer_mode_) {
      uint8_t any_countup = false;
      uint8_t any_new_opens = false;
      
      if (new_reset_press) {
        ResetToInitial();
      } else if (new_play_pause_press) {
        timer_mode_ = paused;
      } else {
        for (int i = 0; i < 8; i++) {
          if (countup == lock_states_[i].lock_mode) {
            if (lock_states_[i].is_on && lock_states_[i].is_open && !lock_states_[i].is_failed) {
              lock_states_[i].lock_mode = complete;
              any_new_opens = true;
            } else {
              lock_states_[i].lock_time = lock_states_[i].lock_time + time_difference;
            }
            any_countup = any_countup || (countup == lock_states_[i].lock_mode);
          }
        }
        if (any_new_opens) {
          // If more than one lock opened, it was on the same cycle,
          // so consider them tied...
          seq_opens_++;
          for (int i = 0; i < 8; i++) {
            if ((complete == lock_states_[i].lock_mode) && (0 == lock_states_[i].order_opened)) {
              lock_states_[i].order_opened = seq_opens_;
            }
          }
        }
        if (!any_countup) {
          timer_mode_ = complete;
        }
      }
    } else if (paused == timer_mode_) {
      if (new_reset_press) {
        ResetToInitial();
      } else if (new_play_pause_press) {
        timer_mode_ = countup;
      }
    } else if (complete == timer_mode_) {
      if (new_reset_press) {
        ResetToInitial();
      }
    }
    
    return lock_states_;
  }
  
  virtual ~Timer() {}

 private:
  Modes timer_mode_;
  static const uint8_t max_locks_ = 8;
  LockState lock_states_[max_locks_];
  uint32_t last_read_time_;
  uint8_t seq_opens_;

  void ResetToInitial() {
    for (int i = 0; i < max_locks_; i++) {
      lock_states_[i].lock_mode = initial;
      lock_states_[i].order_opened = 0;
    }
    timer_mode_ = initial;
    seq_opens_ = 0;
  }

  void SetLockSwitchStates(uint16_t lock_switches) {
    for (int i = 0; i < max_locks_; i++) {
      if (!((lock_switches >> i) & 1u) || !((lock_switches >> (i+8)) & 1u)) {
        lock_states_[i].is_on = true;
      } else {
        lock_states_[i].is_on = false;
      }
      if (((lock_switches >> i) & 1u) && !((lock_switches >> (i+8)) & 1u)) {
        lock_states_[i].is_open = true;
      } else {
        lock_states_[i].is_open = false;
      }
      if (!((lock_switches >> i) & 1u) && !((lock_switches >> (i+8)) & 1u)) {
        lock_states_[i].is_failed = true;
      } else {
        lock_states_[i].is_failed = false;
      }
    }
  }
  
};

#endif  // TOOOLTIMER_HH_
