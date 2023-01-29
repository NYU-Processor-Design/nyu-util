#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <verilated.h>
#include <verilated_cov.h>


class VerilatorCoverageReporter : public Catch::EventListenerBase {
public:
  using Catch::EventListenerBase::EventListenerBase;

  void testCaseEnded(const Catch::TestCaseStats& stats) override {
    Verilated::threadContextp()->coveragep()->write(
        (stats.testInfo->name + ".dat").data());
  }
};

CATCH_REGISTER_LISTENER(VerilatorCoverageReporter)
