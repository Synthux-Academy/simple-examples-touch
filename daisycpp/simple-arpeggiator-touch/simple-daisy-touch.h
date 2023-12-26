#pragma once

#include "daisy.h"
#include "daisy_seed.h"
#include <array>

using namespace daisy;
using namespace seed;

namespace synthux {
namespace simpletouch {

///////////////////////////////////////////////////////////////
//////////////////////// TOUCH SENSOR /////////////////////////

class Touch {
  public:
    Touch():
      _state { 0 },
      _on_touch { nullptr },
      _on_release { nullptr }
      {}

    void Init(DaisySeed hw) {
      // Uncomment if you want to use i2C4
      // Wire.setSCL(D13);
      // Wire.setSDA(D14);
      
      Mpr121I2C::Config config;
      
      int result = _cap.Init(config);
      hw.PrintLine("cap init: %d", result);
    
      if (result != 0) {
        hw.PrintLine("MPR121 config failed");
      }
    }

    // Register note on callback
    void SetOnTouch(void(*on_touch)(uint16_t pad)) {
      _on_touch = on_touch;
    }

    void SetOnRelease(void(*on_release)(uint16_t pad)) {
      _on_release = on_release;
    }

    bool IsTouched(uint16_t pad) {
      return _state & (1 << pad);
    }

    bool HasTouch() {
      return _state > 0;
    }

    void Process() {
        uint16_t pad;
        bool is_touched;
        bool was_touched;
        auto state = _cap.Touched();
        for (uint16_t i = 0; i < 12; i++) {
          pad = 1 << i;
          is_touched = state & pad;
          was_touched = _state & pad;
          if (_on_touch != nullptr && is_touched && !was_touched) {
            _on_touch(i);
          }
          else if (_on_release != nullptr && was_touched && !is_touched) {
            _on_release(i);
          }
        }
        _state = state;
    }

  private:
    uint16_t _state;
    void(*_on_touch)(uint16_t pad);
    void(*_on_release)(uint16_t pad);
    Mpr121I2C _cap;
};

///////////////////////////////////////////////////////////////
//////////////////////////// PINS /////////////////////////////
//Probably can simplify this since Analog and Digital share pins
namespace Analog {
    static constexpr Pin S30 = A0;
    static constexpr Pin S31 =A1;
    static constexpr Pin S32 =A2;
    static constexpr Pin S33 =A3;
    static constexpr Pin S34 =A4;
    static constexpr Pin S35 =A5;
    static constexpr Pin S36 =A6;
    static constexpr Pin S37 =A7;
};

namespace Digital {
    static constexpr Pin S07 = D6;
    static constexpr Pin S08 =D7;
    static constexpr Pin S09 =D8;
    static constexpr Pin S10 =D9;
    static constexpr Pin S30 =D15;
    static constexpr Pin S31 =D16;
    static constexpr Pin S32 =D17;
    static constexpr Pin S33 =D18;
    static constexpr Pin S34 =D19;
    static constexpr Pin S35 =D2;
};


template<class AP, class DP>
class PinST {
public:
  static int a(AP pin) {
    return int(pin);
  }

  static int d(DP pin) {
    return int(pin);
  }
};

//using DaisyPin = PinST<Analog, Digital>;

};
};

#define A(p) synthux::simpletouch::DaisyPin::a(synthux::simpletouch::Analog::p)
#define D(p) synthux::simpletouch::DaisyPin::d(synthux::simpletouch::Digital::p)

#ifdef TEST_PADS
void testPads() {
  for (auto i = 0; i < 12; i++) {
    if (touch.IsTouched(i)) {
      Serial.print("IS TOUCHED ");
      Serial.println(i);
    }
  }
}
#endif

#ifdef TEST_KNOBS
void testKnobs() {
  Serial.print("S30: ");
  Serial.print(analogRead(knob_a));
  Serial.print(" S31: ");
  Serial.print(analogRead(knob_b));
  Serial.print(" S32: ");
  Serial.print(analogRead(knob_c));
  Serial.print(" S33: ");
  Serial.print(analogRead(knob_d));
  Serial.print(" S34: ");
  Serial.print(analogRead(knob_e));
  Serial.print(" S35: ");
  Serial.print(analogRead(knob_f));
  Serial.print(" Fader L: ");
  Serial.print(analogRead(left_fader));
  Serial.print(" Fader R: ");
  Serial.print(analogRead(right_fader));
  Serial.println("");
}
#endif

#ifdef TEST_SWITCHES
void testSwitches() {
  Serial.print("S7: ");
  Serial.print(digitalRead(switch_1_a));
  Serial.print(" S8: ");
  Serial.print(digitalRead(switch_1_b));
  Serial.print(" S9: ");
  Serial.print(digitalRead(switch_2_a));
  Serial.print(" S10: ");
  Serial.print(digitalRead(switch_2_b));
  Serial.println("");
}
#endif
