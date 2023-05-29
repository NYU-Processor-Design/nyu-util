#include <algorithm>
#include <string>

#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <verilated.h>
#include <verilated_cov.h>

class VerilatorCoverageReporter : public Catch::EventListenerBase {
public:
  using Catch::EventListenerBase::EventListenerBase;

  void testCaseStarting(const Catch::TestCaseInfo&) override {
    Verilated::threadContextp()->coveragep()->clear();
  }

  void testCaseEnded(const Catch::TestCaseStats& stats) override {
    Verilated::threadContextp()->coveragep()->write(
        makeFileName(stats.testInfo->name).data());
  }

private:
  std::string makeFileName(const std::string& testName) {
    std::string fileName(testName.size(), '\0');
    std::string unsafe {"/<>:\"\\|?*"};
    std::transform(testName.begin(), testName.end(), fileName.begin(),
        [&](char c) { return unsafe.find(c) == std::string::npos ? c : '_'; });
    return fileName + ".dat";
  }
};

CATCH_REGISTER_LISTENER(VerilatorCoverageReporter)
