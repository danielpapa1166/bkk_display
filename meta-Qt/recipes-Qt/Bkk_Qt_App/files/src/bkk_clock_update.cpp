#include "bkk_clock_update.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

CLockUpdater::CLockUpdater() : clockText(""), errorCode(0) {
	update();
}

CLockUpdater::~CLockUpdater() = default;

std::string CLockUpdater::getClock() const {
	std::lock_guard<std::mutex> lock(clockMutex);
	return clockText;
}

void CLockUpdater::update() {
	const auto now = std::chrono::system_clock::now();
	const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

	std::tm localTime{};
	if (localtime_r(&nowTime, &localTime) == nullptr) {
		std::lock_guard<std::mutex> lock(clockMutex);
		clockText.clear();
		errorCode = 1;
		return;
	}

	std::ostringstream stream;
	stream << std::put_time(&localTime, "%H:%M");

	std::lock_guard<std::mutex> lock(clockMutex);
	clockText = stream.str();
	errorCode = 0;
}
