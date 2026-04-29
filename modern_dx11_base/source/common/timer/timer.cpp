#include "timer.h"

namespace pane::common {

int64_t Timer::EstimateOsSleepOverhead() {
  constexpr int kSamples = 16;
  int64_t overheadTicks = 0;
  for (int i = 0; i < kSamples; ++i) {
    const int64_t before = GetTicks();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const int64_t after = GetTicks();
    overheadTicks += (after - before) - (GetFrequency() / 1000);
  }

  // tick bias
  return (overheadTicks / kSamples) + (GetFrequency() / 2000);
}

int64_t Timer::GetSleepOverhead() {
  // core 0 calibration aka fuck our OS (i hate windows)

  static const int64_t overheadTicks = []() {
    HANDLE thread = GetCurrentThread();
    DWORD_PTR prevAffinity = SetThreadAffinityMask(thread, 1);
    int prevPriority = GetThreadPriority(thread);

    const int64_t result = EstimateOsSleepOverhead();

    SetThreadAffinityMask(thread, prevAffinity);
    SetThreadPriority(thread, prevPriority);
    return result;
  }();
  return overheadTicks;
}

double Timer::GetInvFrequency() {
  static const double invFrequency =
      1000.0 / static_cast<double>(GetFrequency());
  return invFrequency;
}

void Timer::PrecisionSleep(double milliseconds) {
  if (milliseconds <= 0.0)
    return;

  const int64_t frequency = GetFrequency();
  const int64_t overheadTicks = GetSleepOverhead();
  const int64_t targetTicks =
      GetTicks() + static_cast<int64_t>(milliseconds * frequency * 0.001);

  // some cool calculations
  const int64_t kSleepThreshold = overheadTicks + (frequency / 1000);
  const int64_t kYieldThreshold = frequency / 4000; // yield
  const int64_t kPauseThreshold = frequency / 20000; // _mm_pause

  while (true) {
    const int64_t remainingTicks = targetTicks - GetTicks();
    if (remainingTicks <= kSleepThreshold)
      break;

    const int64_t sleepMs = (std::max)(
        int64_t(1), (remainingTicks - overheadTicks) * 1000 / frequency - 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
  }

  while (true) {
    const int64_t remainingTicks = targetTicks - GetTicks();
    if (remainingTicks <= kYieldThreshold)
      break;
    std::this_thread::yield();
  }

  while (true) {
    const int64_t remainingTicks = targetTicks - GetTicks();
    if (remainingTicks <= kPauseThreshold)
      break;
    _mm_pause();
    _mm_pause();
    _mm_pause();
    _mm_pause();
  }

  while (GetTicks() < targetTicks) {
    _mm_pause();
  }
}

double Timer::GetTime() {
  return static_cast<double>(GetTicks()) * GetInvFrequency();
}

// QPC around ~100 nanoseconds
int64_t Timer::GetTicks() {
  LARGE_INTEGER ticks;
  QueryPerformanceCounter(&ticks);
  return ticks.QuadPart;
}

// Cache frequency
int64_t Timer::GetFrequency() {
  static const int64_t frequency = []() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
  }();
  return frequency;
}

} // namespace pane::common