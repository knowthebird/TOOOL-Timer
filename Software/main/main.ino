#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_MCP23X17.h>
#include "TooolTimer.hh"
#include <Math.h>

const uint8_t ShackleOpenedLock1 = 0;  // GPA0 (21) of the MCP23017
const uint8_t ShackleOpenedLock2 = 1;  // GPA1 (22) of the MCP23017
const uint8_t ShackleOpenedLock3 = 2;  // GPA2 (23) of the MCP23017
const uint8_t ShackleOpenedLock4 = 3;  // GPA3 (24) of the MCP23017
const uint8_t ShackleOpenedLock5 = 4;  // GPA4 (25) of the MCP23017
const uint8_t ShackleOpenedLock6 = 5;  // GPA5 (26) of the MCP23017
const uint8_t ShackleOpenedLock7 = 6;  // GPA6 (27) of the MCP23017
const uint8_t ShackleOpenedLock8 = 7;  // GPA7 (28) of the MCP23017
const uint8_t ShackleClosedLock1 = 8;  // GPB0  (1) of the MCP23017
const uint8_t ShackleClosedLock2 = 9;  // GPB1  (2) of the MCP23017
const uint8_t ShackleClosedLock3 = 10; // GPB2  (3) of the MCP23017
const uint8_t ShackleClosedLock4 = 11; // GPB3  (4) of the MCP23017
const uint8_t ShackleClosedLock5 = 12; // GPB4  (5) of the MCP23017
const uint8_t ShackleClosedLock6 = 13; // GPB5  (6) of the MCP23017
const uint8_t ShackleClosedLock7 = 14; // GPB6  (7) of the MCP23017
const uint8_t ShackleClosedLock8 = 15; // GPB7  (8) of the MCP23017

const uint8_t ModeButtonLight      = 0;  // GPA0 (21) of the MCP23017
const uint8_t PlayPauseButtonLight = 1;  // GPA1 (22) of the MCP23017
const uint8_t ResetButtonLight     = 2;  // GPA2 (23) of the MCP23017
const uint8_t SpareIo1             = 3;  // GPA3 (24) of the MCP23017
const uint8_t SpareIo2             = 4;  // GPA4 (25) of the MCP23017
const uint8_t SpareIo3             = 5;  // GPA5 (26) of the MCP23017
const uint8_t SpareIo4             = 6;  // GPA6 (27) of the MCP23017
const uint8_t SpareIo5             = 7;  // GPA7 (28) of the MCP23017
const uint8_t ModeButtonNC         = 8;  // GPB0  (1) of the MCP23017
const uint8_t PlayPauseButtonNC    = 9;  // GPB1  (2) of the MCP23017
const uint8_t ResetButtonNC        = 10; // GPB2  (3) of the MCP23017
const uint8_t ModeButtonNO         = 11; // GPB3  (4) of the MCP23017
const uint8_t PlayPauseButtonNO    = 12; // GPB4  (5) of the MCP23017
const uint8_t ResetButtonNO        = 13; // GPB5  (6) of the MCP23017
const uint8_t SpareIo12            = 14; // GPB6  (7) of the MCP23017
const uint8_t SpareIo13            = 15; // GPB7  (8) of the MCP23017

Adafruit_MCP23X17 IoExpander1;
Adafruit_MCP23X17 IoExpander2;

Button ModeButton(ModeButtonNO, ModeButtonNC);
Button PlayPauseButton(PlayPauseButtonNO, PlayPauseButtonNC);
Button ResetButton(ResetButtonNO, ResetButtonNC, INPUT_PULLUP, 2000);

const uint8_t Disp_Addr[8] = {0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77};
Adafruit_7segment Display[8] = Adafruit_7segment();

Timer TooolTimer;

uint32_t last_msg_change_time_;
uint8_t msg_number_to_send;

void setup() {
  Serial.begin(115200);
  Serial.println("The Open Organization Of Lockpickers (TOOOL) timer.");
  Serial.println("Starting...");
  for (int i = 0; i < 8; i++) {
    if (!Display[i].begin(Disp_Addr[i])) {
      Serial.print("Failed to start Display: ");
      Serial.println(i);
      while (1);
    } else {
      Display[i].print("8888");
      Display[i].drawColon(true);
      Display[i].writeDisplay(); 
    }
  }
  if (IoExpander1.begin_I2C(0x20)) {
    IoExpander1.pinMode(ModeButtonLight,         OUTPUT);
    IoExpander1.pinMode(PlayPauseButtonLight,    OUTPUT);
    IoExpander1.pinMode(ResetButtonLight,        OUTPUT);
    IoExpander1.pinMode(SpareIo1,          INPUT_PULLUP);
    IoExpander1.pinMode(SpareIo2,          INPUT_PULLUP);
    IoExpander1.pinMode(SpareIo3,          INPUT_PULLUP);
    IoExpander1.pinMode(SpareIo4,          INPUT_PULLUP);
    IoExpander1.pinMode(SpareIo5,          INPUT_PULLUP);
    IoExpander1.pinMode(ModeButtonNC,      INPUT_PULLUP);
    IoExpander1.pinMode(PlayPauseButtonNC, INPUT_PULLUP);
    IoExpander1.pinMode(ResetButtonNC,     INPUT_PULLUP);
    IoExpander1.pinMode(ModeButtonNO,      INPUT_PULLUP);
    IoExpander1.pinMode(PlayPauseButtonNO, INPUT_PULLUP);
    IoExpander1.pinMode(ModeButtonNO,      INPUT_PULLUP);
    IoExpander1.pinMode(SpareIo12,         INPUT_PULLUP);
    IoExpander1.pinMode(SpareIo13,         INPUT_PULLUP);

    IoExpander1.digitalWrite(ModeButtonLight, HIGH);
    IoExpander1.digitalWrite(PlayPauseButtonLight, HIGH);
    IoExpander1.digitalWrite(ResetButtonLight, HIGH);
    
    Serial.print("IoExpander1: ");
    Serial.println(IoExpander1.readGPIOAB());
  } else {
    Serial.println("Failed to start IoExpander1.");
    while (1);
  }
  if (IoExpander2.begin_I2C(0x21)) {
    IoExpander2.pinMode(ShackleOpenedLock1, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleOpenedLock2, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleOpenedLock3, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleOpenedLock4, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleOpenedLock5, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleOpenedLock6, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleOpenedLock7, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleOpenedLock8, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock1, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock2, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock3, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock4, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock5, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock6, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock7, INPUT_PULLUP);
    IoExpander2.pinMode(ShackleClosedLock8, INPUT_PULLUP);
    Serial.print("IoExpander2: ");
    Serial.println(IoExpander2.readGPIOAB());
  } else {
    Serial.println("Failed to start IoExpander2.");
    while (1);
  }

  TooolTimer.Initialize();
  
  Serial.println("Startup Success");

  delay(3000);
  for (int i = 0; i <8; i++) {
    Display[i].clear();
    Display[i].writeDisplay();
  }

  last_msg_change_time_ = millis();
  msg_number_to_send = 1;
}

void DisplayTimeDetailed(uint8_t idx, LockState* lock_states) {
  double temp = lock_states[idx].lock_time / (1000.0*60.0*60.0);
  uint8_t time_hour = (uint8_t)temp;
  
  temp = fmod(temp,1.0) * 60.0;
  uint8_t time_min = (uint8_t)temp;

  temp = fmod(temp,1.0) * 60.0;
  uint8_t time_sec = (uint8_t)temp;

  temp = fmod(temp,1.0) * 100.0;
  uint8_t time_centisec = (uint8_t)temp;
   
  if (time_hour >= 1) {
    Display[idx].drawColon(true);
    if (time_hour < 10) {
      Display[idx].writeDigitNum(0, 0);
      Display[idx].writeDigitNum(1, time_hour);
    } else {
      Display[idx].writeDigitNum(0, (uint8_t)(time_hour / 10));
      Display[idx].writeDigitNum(1, (uint8_t)(time_hour % 10));
    }
    if (time_min < 10) {
      Display[idx].writeDigitNum(3, 0);
      Display[idx].writeDigitNum(4, time_min);
    } else {
      Display[idx].writeDigitNum(3, (uint8_t)(time_min / 10));
      Display[idx].writeDigitNum(4, (uint8_t)(time_min % 10));
    }
  } else if (time_min >= 1) {
    Display[idx].drawColon(true);
    if (time_min < 10) {
      Display[idx].writeDigitNum(0, 0);
      Display[idx].writeDigitNum(1, time_min);
    } else {
      Display[idx].writeDigitNum(0, (uint8_t)(time_min / 10));
      Display[idx].writeDigitNum(1, (uint8_t)(time_min % 10));
    }
    if (time_sec < 10) {
      Display[idx].writeDigitNum(3, 0);
      Display[idx].writeDigitNum(4, time_sec);
    } else {
      Display[idx].writeDigitNum(3, (uint8_t)(time_sec / 10));
      Display[idx].writeDigitNum(4, (uint8_t)(time_sec % 10));
    }
  } else {
    if (time_sec < 10) {
      Display[idx].writeDigitNum(0, 0);
      Display[idx].writeDigitNum(1, time_sec, true);
    } else {
      Display[idx].writeDigitNum(0, (uint8_t)(time_sec / 10));
      Display[idx].writeDigitNum(1, (uint8_t)(time_sec % 10), true);
    }
    if (time_centisec < 10) {
      Display[idx].writeDigitNum(3, 0);
      Display[idx].writeDigitNum(4, time_centisec);
    } else {
      Display[idx].writeDigitNum(3, (uint8_t)(time_centisec / 10));
      Display[idx].writeDigitNum(4, (uint8_t)(time_centisec % 10));
    }
  }
}

void loop() {
  uint16_t button_states = IoExpander1.readGPIOAB();
  uint16_t lock_switches = IoExpander2.readGPIOAB();

  if (ModeButton.IsPressed(button_states)) {
    IoExpander1.digitalWrite(ModeButtonLight, HIGH);
  } else {
    IoExpander1.digitalWrite(ModeButtonLight, LOW);
  }
  if (PlayPauseButton.IsPressed(button_states)) {
    IoExpander1.digitalWrite(PlayPauseButtonLight, HIGH);
  } else {
    IoExpander1.digitalWrite(PlayPauseButtonLight, LOW);
  }
  if (ResetButton.IsPressed(button_states)) {
    IoExpander1.digitalWrite(ResetButtonLight, HIGH);
  } else {
    IoExpander1.digitalWrite(ResetButtonLight, LOW);
  }

  LockState* lock_states = TooolTimer.Step(lock_switches, ModeButton.IsNewPress(button_states), PlayPauseButton.IsNewPress(button_states), ResetButton.IsNewPress(button_states));

  uint32_t current_time = millis();
  if (current_time > last_msg_change_time_ + 1000) {
    last_msg_change_time_ = current_time;
    if (1 == msg_number_to_send) {
      msg_number_to_send = 2;
    } else if (2 == msg_number_to_send) {
      msg_number_to_send = 3;
    } else if (3 == msg_number_to_send) {
      msg_number_to_send = 1;
    }
  }
  
  for (int i = 0; i < 8; i++) {
    Display[i].clear();
    
    if (initial == lock_states[i].lock_mode) {
      if (msg_number_to_send / 2) {
        if (lock_states[i].is_open) {
          Display[i].println("Shut");
        } else if (lock_states[i].is_on) {
          Display[i].println("Play");
        } else {
          Display[i].println("Off");
        }
      } else {
        if (lock_states[i].is_open) {
          Display[i].println("Bolt");
        } else if (lock_states[i].is_on) {
          Display[i].println("Play");
        } else {
          Display[i].println("Off");
        }
      }
    }
    if (starting == lock_states[i].lock_mode) {
      Display[i].println(lock_states[i].lock_time/1000);
    }
    if (countup == lock_states[i].lock_mode) {
      if (!lock_states[i].is_on) {
        if (1 == msg_number_to_send) {
          Display[i].println("Bad"); 
        } else if (2 == msg_number_to_send) {
          Display[i].println("Line"); 
        } else {
          DisplayTimeDetailed(i, lock_states);
        }
      } else {
        DisplayTimeDetailed(i, lock_states);
      }

    }
    if (complete == lock_states[i].lock_mode) {
      if (1 == msg_number_to_send) {
        if (1 == lock_states[i].order_opened) {
          Display[i].println("1st"); 
        } else if (2 == lock_states[i].order_opened) {
          Display[i].println("2nd"); 
        } else if (3 == lock_states[i].order_opened) {
          Display[i].println("3rd"); 
        }  else if (4 == lock_states[i].order_opened) {
          Display[i].println("4th"); 
        }  else if (5 == lock_states[i].order_opened) {
          Display[i].println("5th"); 
        }  else if (6 == lock_states[i].order_opened) {
          Display[i].println("6th"); 
        }  else if (7 == lock_states[i].order_opened) {
          Display[i].println("7th"); 
        }  else if (8 == lock_states[i].order_opened) {
          Display[i].println("8th"); 
        }
      } else if (2 == msg_number_to_send) {
        Display[i].println("OPEN");
      } else {
        DisplayTimeDetailed(i, lock_states);
      }
    }
    if (lock_states[i].is_failed) {
      Display[i].println("Err"); 
    }
    Display[i].writeDisplay(); 

  }
  

}
