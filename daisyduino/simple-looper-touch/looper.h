#pragma once
#include "buf.h"
#include "win.h"
#include <array>

namespace synthux {

template<size_t win_slope = 192>
class Looper {
  public:
    Looper():
    _buffer             { nullptr },
    _delta              { 1.f },
    _volume             { 1.f },
    _release_kof        { 0.f },
    _loop_start         { 0 },
    _loop_start_offset  { 0 },
    _win_per_loop       { 0 },
    _win_current        { 0 },
    _is_playing         { false },
    _is_gate_open       { false },
    _is_reverse         { false },
    _mode               { Mode::loop }
    {}

    void Init(Buffer* buffer) {
        _buffer = buffer;
    }

    void SetGateOpen(bool open) {
      if (open && !_is_gate_open) {
        _volume = 1.f;
        _win_current = 0;
        _Activate(0);
        _is_playing = true;
      }
      _is_gate_open = open;
    }

    bool IsPlaying() {
      return _is_playing;
    }

    void SetRelease(const float value) {
      if (value <= 0.002f) {
        _mode = Mode::one_shot;
      }
      else if (value >= 0.998f) {
        _mode = Mode::loop;
      }
      else {
        _mode = Mode::release;
        auto v = fmap(value, 0, 1, Mapping::EXP);
        _release_kof = (1.f - v) * 0.000139f * (1 - 0.9 * value);
      }
    }

    void SetSpeed(const float value) {
        if (value > 0.23 && value < 0.27) {
          _delta = 1.f;
          _is_reverse = true;
        }
        else if (value > 0.48 && value < 0.52) {
          _delta = 0.f;
        }
        else if (value > 0.73 && value < 0.77) {
          _delta = 1.f;
          _is_reverse = false;
        }
        else if (value < 0.5) {
          _delta = (0.5f - value) * 4.f;
          _is_reverse = true;
        }
        else {
          _delta = (value - 0.5) * 4.f;
          _is_reverse = false;
        }
    }

    void SetLoop(const float loop_start, const float loop_length) {
      _loop_start = static_cast<size_t>(loop_start * _buffer->Length());
      // Quantize loop length to the window slope. Minimum is 2 slopes = 1 window.
      // This gives 4ms precision (win_slope = 192), which is 250 points on the turn.
      // Speed affects loop length. The higher is the speed the smaller is length.
      auto new_length = static_cast<size_t>(loop_length * _buffer->Length() / _delta);
      _win_per_loop = std::max(static_cast<size_t>(new_length / win_slope), static_cast<size_t>(2));
    }
  
    void Process(float& out0, float& out1) {
      out0 = 0.f;
      out1 = 0.f;

      if (!_is_playing) return;
      
      auto wrap = false;
      for (auto& w: _wins) {
        if (!w.IsHalf()) continue;
        if (_win_current >= _win_per_loop - 2) { // we're instersted in the last but one window
          if (_mode == Mode::one_shot) {
            _win_current = 0;
            continue;
          }
          wrap = true;
        }

        auto start = w.PlayHead();
        if (wrap) {
          start = _is_reverse ? _win_per_loop * win_slope - 1 : 0;
        }
        if (_Activate(start)) {
          _win_current = wrap ? 0 : _win_current + 1;
          break;
        }
      }

      if (!_is_gate_open) {
        if (_mode == Mode::release) _volume -= _release_kof * _volume;
        if (_volume <= .02f) {
          _Stop();
          return;
        }
      }

      auto w_out0 = 0.f;
      auto w_out1 = 0.f;
      for (auto& w: _wins) {
          if (!w.IsActive()) continue;
          w_out0 = 0.f;
          w_out1 = 0.f;
          w.Process(_buffer, w_out0, w_out1);
          out0 += w_out0 * _volume;
          out1 += w_out1 * _volume;
      }
    }

private:
    bool _Activate(float play_head) {
      for (auto& w: _wins) {
          if (!w.IsActive()) {
              auto delta = _is_reverse ? -_delta : _delta;
              w.Activate(play_head, delta, _loop_start + _loop_start_offset);
              return true;
          }
      }
      return false;
    }

    void _Stop() {
      _is_playing = false;
      for (auto& w: _wins) w.Deactivate();
    }

    enum class Mode {
      one_shot,
      loop,
      release
    };

    static constexpr size_t kMinLoopLength = 2 * win_slope;

    Buffer* _buffer;
    std::array<Window<win_slope>, 2> _wins;

    float _delta;
    float _volume;
    float _release_kof;
    float _last_playhead;
    size_t _loop_start;
    int32_t _loop_start_offset;
    size_t _win_per_loop;
    size_t _win_current;
    Mode _mode;
    bool _is_playing;
    bool _is_gate_open;
    bool _is_reverse;
    
};
};
