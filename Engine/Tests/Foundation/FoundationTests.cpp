#include <sstream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include "Aquila/Foundation/Timer.h"
#include "Aquila/Foundation/Log.h"
#include "Aquila/Foundation/Profiler.h"

using namespace Aquila::Foundation;

static void RunFrame(const std::string &section = "Work",
					 std::chrono::milliseconds sleep = std::chrono::milliseconds(1)) {
	Profiler::Get()->BeginFrame();
	Profiler::Get()->BeginSection(section);
	std::this_thread::sleep_for(sleep);
	Profiler::Get()->EndSection();
	Profiler::Get()->EndFrame();
}

#define RESET() Profiler::Get()->Reset()
#define PROFILE_INIT() Profiler::Init();
#define PROFILE_SHUTDOWN() Profiler::Shutdown();

TEST_SUITE("Timer tests") {
	TEST_CASE("Now() returns a valid time point") {
		auto before = Clock::now();
		auto now = Now();
		auto after = Clock::now();

		CHECK(now >= before);
		CHECK(now <= after);
	}

	TEST_CASE("ElapsedSeconds returns correct duration") {
		auto start = Now();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		auto end = Now();

		double elapsed = ElapsedSeconds(start, end);
		CHECK(elapsed >= 0.08);
		CHECK(elapsed <= 0.5);
	}

	TEST_CASE("ElapsedMilliseconds returns correct duration") {
		auto start = Now();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		auto end = Now();

		double elapsed = ElapsedMilliseconds(start, end);
		CHECK(elapsed >= 90.0);
		CHECK(elapsed <= 500.0);
	}

	TEST_CASE("ElapsedSeconds and ElapsedMilliseconds are consistent") {
		auto start = Now();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		auto end = Now();

		double secs = ElapsedSeconds(start, end);
		double millis = ElapsedMilliseconds(start, end);

		CHECK(millis == doctest::Approx(secs * 1000.0).epsilon(0.01));
	}

	TEST_CASE("ElapsedSeconds with same start and end is ~zero") {
		auto t = Now();
		double elapsed = ElapsedSeconds(t, t);
		CHECK(elapsed == doctest::Approx(0.0).epsilon(1e-9));
	}

	TEST_CASE("GetTimeSinceStart returns positive elapsed time") {
		auto start = Now();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		double elapsed = GetTimeSinceStart(start);
		CHECK(elapsed >= 0.04);
		CHECK(elapsed <= 0.5);
	}

	TEST_CASE("Stopwatch starts on construction") {
		Stopwatch sw;
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		CHECK(sw.GetElapsedTime() >= 0.04f);
	}

	TEST_CASE("Stopwatch::GetElapsedTime increases over time") {
		Stopwatch sw;

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		float t1 = sw.GetElapsedTime();

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		float t2 = sw.GetElapsedTime();

		CHECK(t2 > t1);
	}

	TEST_CASE("Stopwatch::GetDeltaTime is 0 before first Tick") {
		Stopwatch sw;
		CHECK(sw.GetDeltaTime() == doctest::Approx(0.0f));
	}

	TEST_CASE("Stopwatch::Tick updates delta time") {
		Stopwatch sw;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		sw.Tick();

		CHECK(sw.GetDeltaTime() >= 0.09f);
		CHECK(sw.GetDeltaTime() <= 0.5f);
	}

	TEST_CASE("Stopwatch::Tick measures time between ticks, not since start") {
		Stopwatch sw;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		sw.Tick();

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		sw.Tick();

		CHECK(sw.GetDeltaTime() >= 0.04f);
		CHECK(sw.GetDeltaTime() <= 0.2f);
	}

	TEST_CASE("Stopwatch::Start resets elapsed time") {
		Stopwatch sw;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		sw.Start();
		float elapsed = sw.GetElapsedTime();

		CHECK(elapsed < 0.05f);
	}

	TEST_CASE("Stopwatch::Start resets delta time to 0") {
		Stopwatch sw;
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		sw.Tick();

		sw.Start();
		CHECK(sw.GetDeltaTime() == doctest::Approx(0.0f));
	}
}

TEST_SUITE("Logger tests") {
	TEST_CASE("Log debug writes message") {
		std::ostringstream capture;
		Logger::SetSink(&capture);

		Logger::LogDebug("Test message");

		Logger::SetSink(nullptr);
		CHECK(capture.str().find("Test message") != std::string::npos);
	}

	TEST_CASE("Log level filters lower levels") {
		std::ostringstream capture;
		Logger::SetSink(&capture);
		Logger::SetLogLevel(LogLevel::Warning);

		Logger::LogDebug("Should not appear");

		Logger::SetSink(nullptr);
		CHECK(capture.str().empty());
	}

	TEST_CASE("Profiler is enabled by default") {
		PROFILE_INIT();
		RESET();
		CHECK(Profiler::Get()->IsEnabled());
	}

	TEST_CASE("SetEnabled toggles the flag") {
		RESET();
		Profiler::Get()->SetEnabled(false);
		CHECK_FALSE(Profiler::Get()->IsEnabled());
		Profiler::Get()->SetEnabled(true);
		CHECK(Profiler::Get()->IsEnabled());
	}

	TEST_CASE("Frame counters start at zero after Reset") {
		RESET();
		CHECK(Profiler::Get()->GetFrameCount() == 0u);
		CHECK(Profiler::Get()->GetFrameNumber() == 0u);
	}

	TEST_CASE("BeginFrame increments FrameNumber; EndFrame increments FrameCount") {
		RESET();
		RunFrame();
		CHECK(Profiler::Get()->GetFrameNumber() == 1u);
		CHECK(Profiler::Get()->GetFrameCount() == 1u);

		RunFrame();
		CHECK(Profiler::Get()->GetFrameNumber() == 2u);
		CHECK(Profiler::Get()->GetFrameCount() == 2u);
	}

	TEST_CASE("FrameDuration is positive after a frame") {
		RESET();
		RunFrame("W", std::chrono::milliseconds(2));
		CHECK(Profiler::Get()->GetFrameDuration() > 0.0);
	}

	TEST_CASE("FPS is consistent with frame duration (fps * dur ~ 1000 ms)") {
		RESET();
		RunFrame("W", std::chrono::milliseconds(10));
		double dur = Profiler::Get()->GetFrameDuration();
		double fps = Profiler::Get()->GetFPS();
		CHECK(fps * dur == doctest::Approx(1000.0).epsilon(0.05));
	}

	TEST_CASE("Named section appears in CurrentFrameEntries") {
		RESET();
		Profiler::Get()->BeginFrame();
		Profiler::Get()->BeginSection("Render");
		Profiler::Get()->EndSection();
		Profiler::Get()->EndFrame();

		const auto &entries = Profiler::Get()->GetCurrentFrameEntries();
		REQUIRE(entries.size() == 1u);
		CHECK(entries[0].name == "Render");
	}

	TEST_CASE("Section duration is non-negative") {
		RESET();
		RunFrame("Physics");
		const auto &entries = Profiler::Get()->GetCurrentFrameEntries();
		REQUIRE_FALSE(entries.empty());
		CHECK(entries[0].duration >= 0.0);
	}

	TEST_CASE("Multiple sections in one frame are all recorded") {
		RESET();
		Profiler::Get()->BeginFrame();
		for (auto name : { "A", "B", "C" }) {
			Profiler::Get()->BeginSection(name);
			Profiler::Get()->EndSection();
		}
		Profiler::Get()->EndFrame();

		CHECK(Profiler::Get()->GetCurrentFrameEntries().size() == 3u);
	}

	TEST_CASE("Nested sections get correct depth values") {
		RESET();
		Profiler::Get()->BeginFrame();
		Profiler::Get()->BeginSection("Outer");
		Profiler::Get()->BeginSection("Inner");
		Profiler::Get()->EndSection();
		Profiler::Get()->EndSection();
		Profiler::Get()->EndFrame();

		const auto &e = Profiler::Get()->GetCurrentFrameEntries();
		REQUIRE(e.size() == 2u);
		CHECK(e[0].name == "Outer");
		CHECK(e[0].depth == 0);
		CHECK(e[1].name == "Inner");
		CHECK(e[1].depth == 1);
	}

	TEST_CASE("Stats accumulate correctly over multiple frames") {
		RESET();
		for (int i = 0; i < 5; ++i)
			RunFrame("Loop");

		ProfilerEntry stats;
		REQUIRE(Profiler::Get()->GetSectionStats("Loop", stats));
		CHECK(stats.frameCount == 5u);
		CHECK(stats.avgDuration >= 0.0);
		CHECK(stats.minDuration <= stats.maxDuration);
	}

	TEST_CASE("GetSectionStats returns false for unknown section") {
		RESET();
		ProfilerEntry dummy;
		CHECK_FALSE(Profiler::Get()->GetSectionStats("DoesNotExist", dummy));
	}

	TEST_CASE("FrameHistory grows by one entry per frame") {
		RESET();
		RunFrame();
		RunFrame();
		RunFrame();
		CHECK(Profiler::Get()->GetFrameHistory().size() == 3u);
	}

	TEST_CASE("FrameStatsHistory size matches FrameHistory size") {
		RESET();
		for (int i = 0; i < 7; ++i)
			RunFrame();
		CHECK(Profiler::Get()->GetFrameStatsHistory().size() == Profiler::Get()->GetFrameHistory().size());
	}

	TEST_CASE("FrameTimeHistory ring buffer is 120 elements") {
		RESET();
		CHECK(Profiler::Get()->GetFrameTimeHistory().size() == 120u);
	}

	TEST_CASE("FrameTimeHistory records non-zero values after frames") {
		RESET();
		for (int i = 0; i < 3; ++i)
			RunFrame("W", std::chrono::milliseconds(1));

		int nonzero = 0;
		for (float v : Profiler::Get()->GetFrameTimeHistory())
			if (v > 0.0f)
				++nonzero;
		CHECK(nonzero == 3);
	}

	TEST_CASE("Reset clears all accumulated state") {
		RESET();
		for (int i = 0; i < 10; ++i)
			RunFrame("X");

		Profiler::Get()->Reset();

		CHECK(Profiler::Get()->GetFrameCount() == 0u);
		CHECK(Profiler::Get()->GetFrameNumber() == 0u);
		CHECK(Profiler::Get()->GetStats().empty());
		CHECK(Profiler::Get()->GetFrameHistory().empty());
		CHECK(Profiler::Get()->GetFrameStatsHistory().empty());
		CHECK(Profiler::Get()->GetCurrentFrameEntries().empty());
		CHECK(Profiler::Get()->GetBottlenecks().empty());

		for (float v : Profiler::Get()->GetFrameTimeHistory())
			CHECK(v == doctest::Approx(0.0f));
	}

	TEST_CASE("EndSection on empty stack does not crash") {
		RESET();
		Profiler::Get()->BeginFrame();
		CHECK_NOTHROW(Profiler::Get()->EndSection());
		Profiler::Get()->EndFrame();
	}

	TEST_CASE("Section consuming >20% of frame time is flagged as bottleneck") {
		RESET();
		for (int i = 0; i < 5; ++i) {
			Profiler::Get()->BeginFrame();
			Profiler::Get()->BeginSection("HeavyWork");
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			Profiler::Get()->EndSection();
			Profiler::Get()->EndFrame();
		}

		const auto &bns = Profiler::Get()->GetBottlenecks();
		bool found = std::find(bns.begin(), bns.end(), "HeavyWork") != bns.end();
		CHECK(found);
	}

	TEST_CASE("ProfileSection RAII calls Begin/End automatically") {
		RESET();
		Profiler::Get()->BeginFrame();
		{
			ProfileSection ps("RAIISection");
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		Profiler::Get()->EndFrame();

		const auto &entries = Profiler::Get()->GetCurrentFrameEntries();
		REQUIRE(entries.size() == 1u);
		CHECK(entries[0].name == "RAIISection");
		CHECK(entries[0].duration > 0.0);
	}

	TEST_CASE("ProfileSection does nothing when profiler is disabled") {
		RESET();
		Profiler::Get()->SetEnabled(false);

		Profiler::Get()->BeginFrame();
		{
			ProfileSection ps("Ignored");
		}
		Profiler::Get()->EndFrame();

		CHECK(Profiler::Get()->GetCurrentFrameEntries().empty());
		Profiler::Get()->SetEnabled(true);
	}

	TEST_CASE("Section records the calling thread id") {
		RESET();
		RunFrame("TID");
		const auto &e = Profiler::Get()->GetCurrentFrameEntries();
		REQUIRE_FALSE(e.empty());
		CHECK(e[0].threadId == std::this_thread::get_id());
	}

	TEST_CASE("Same section name always produces the same color hash") {
		RESET();
		RunFrame("Stable");
		auto color1 = Profiler::Get()->GetCurrentFrameEntries()[0].color;

		RESET();
		RunFrame("Stable");
		auto color2 = Profiler::Get()->GetCurrentFrameEntries()[0].color;

		CHECK(color1 == color2);
	}

	TEST_CASE("Different section names produce different color hashes") {
		RESET();
		Profiler::Get()->BeginFrame();
		Profiler::Get()->BeginSection("Alpha");
		Profiler::Get()->EndSection();
		Profiler::Get()->BeginSection("Beta");
		Profiler::Get()->EndSection();
		Profiler::Get()->EndFrame();

		const auto &e = Profiler::Get()->GetCurrentFrameEntries();
		REQUIRE(e.size() == 2u);
		CHECK(e[0].color != e[1].color);

		PROFILE_SHUTDOWN(); // ! TODO: THIS SHOULD ALWAYS BE IN THE LAST TEST FOR THE PROFILER
	}
}
