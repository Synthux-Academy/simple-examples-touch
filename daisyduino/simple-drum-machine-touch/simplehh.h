#pragma once;
#include "DaisyDuino.h"

namespace synthux {
  class SimpleHH {
    public:
      void Init(float sample_rate) {
        _hh.Init(sample_rate);
        _hh.SetDecay(0.7);
        _hh.SetTone(0.8);
        _hh.SetNoisiness(0.7);
      }

      float Process(bool gate) {
        return _hh.Process(gate);
      }

      void SetSound(float value) {
    
      }

    private:
      HiHat<> _hh;
  };
};