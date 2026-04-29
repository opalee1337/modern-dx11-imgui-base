#pragma once
#include "pch.h"

namespace pane::common {

class Timer {
public:
  static void PrecisionSleep(double milliseconds);
  static void Init() { (void)GetSleepOverhead(); }

  // [[nodiscard]] against nop
  [[nodiscard]] static double GetTime();
  [[nodiscard]] static int64_t GetTicks();
  [[nodiscard]] static int64_t GetFrequency();

private:
  [[nodiscard]] static int64_t EstimateOsSleepOverhead();
  [[nodiscard]] static int64_t GetSleepOverhead();
  [[nodiscard]] static double GetInvFrequency();
};

} // namespace pane::common