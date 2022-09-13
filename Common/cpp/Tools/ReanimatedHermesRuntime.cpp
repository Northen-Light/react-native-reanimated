#include "ReanimatedHermesRuntime.h"

#include <cxxreact/MessageQueueThread.h>
#include <jsi/decorator.h>
#include <jsi/jsi.h>

#include <memory>
#include <utility>

#if __has_include(<reacthermes/HermesExecutorFactory.h>)
#include <reacthermes/HermesExecutorFactory.h>
#else // __has_include(<hermes/hermes.h>) || ANDROID
#include <hermes/hermes.h>
#endif

#include <hermes/inspector/RuntimeAdapter.h>
#include <hermes/inspector/chrome/Registration.h>

namespace reanimated {

using namespace facebook;
using namespace react;

class HermesExecutorRuntimeAdapter
    : public facebook::hermes::inspector::RuntimeAdapter {
 public:
  HermesExecutorRuntimeAdapter(
      facebook::hermes::HermesRuntime &hermesRuntime,
      std::shared_ptr<MessageQueueThread> thread)
      : hermesRuntime_(hermesRuntime), thread_(std::move(thread)) {}

  virtual ~HermesExecutorRuntimeAdapter() {
    thread_->quitSynchronous();
  }

  facebook::jsi::Runtime &getRuntime() override {
    return hermesRuntime_;
  }

  facebook::hermes::debugger::Debugger &getDebugger() override {
    return hermesRuntime_.getDebugger();
  }

  void tickleJs() override {}

 public:
  facebook::hermes::HermesRuntime &hermesRuntime_;
  std::shared_ptr<MessageQueueThread> thread_;
};

ReanimatedHermesRuntime::ReanimatedHermesRuntime(
    std::unique_ptr<jsi::Runtime> runtime,
    facebook::hermes::HermesRuntime &hermesRuntime,
    std::shared_ptr<MessageQueueThread> jsQueue)
    : jsi::WithRuntimeDecorator<ReentrancyCheck>(*runtime, reentrancyCheck_),
      runtime_(std::move(runtime)),
      hermesRuntime_(hermesRuntime) {
#if HERMES_ENABLE_DEBUGGER
  std::shared_ptr<facebook::hermes::HermesRuntime> rt(runtime_, &hermesRuntime);
  auto adapter =
      std::make_unique<HermesExecutorRuntimeAdapter>(hermesRuntime, jsQueue);
  facebook::hermes::inspector::chrome::enableDebugging(
      std::move(adapter), "Reanimated Runtime");
#else
  // This is to prevent unused variable warnings
  (void)messageQueueThread;
#endif
}

ReanimatedHermesRuntime::~ReanimatedHermesRuntime() {
#if HERMES_ENABLE_DEBUGGER
  facebook::hermes::inspector::chrome::disableDebugging(hermesRuntime_);
#endif
}

} // namespace reanimated
