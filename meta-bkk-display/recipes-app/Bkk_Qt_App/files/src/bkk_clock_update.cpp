#include "bkk_clock_update.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {

int getLastSundayOfMonthUtc(const int year, const int monthOneBased) {
	std::tm monthEndUtc{};
	monthEndUtc.tm_year = year - 1900;
	monthEndUtc.tm_mon = monthOneBased;
	monthEndUtc.tm_mday = 0;
	monthEndUtc.tm_hour = 12;

	const std::time_t monthEndTime = timegm(&monthEndUtc);
	std::tm resolvedMonthEndUtc{};
	gmtime_r(&monthEndTime, &resolvedMonthEndUtc);

	return resolvedMonthEndUtc.tm_mday - resolvedMonthEndUtc.tm_wday;
}

bool isCentralEuropeanSummerTimeUtc(const std::tm& utcTime) {
	const int year = utcTime.tm_year + 1900;
	const int month = utcTime.tm_mon + 1;
	const int day = utcTime.tm_mday;
	const int hour = utcTime.tm_hour;

	const int marchSwitchDay = getLastSundayOfMonthUtc(year, 3);
	const int octoberSwitchDay = getLastSundayOfMonthUtc(year, 10);

	const bool afterMarchSwitch =
		(month > 3) ||
		(month == 3 && (day > marchSwitchDay || (day == marchSwitchDay && hour >= 1)));

	const bool beforeOctoberSwitch =
		(month < 10) ||
		(month == 10 && (day < octoberSwitchDay || (day == octoberSwitchDay && hour < 1)));

	return afterMarchSwitch && beforeOctoberSwitch;
}

} // namespace

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

	std::tm utcTime{};
	if (gmtime_r(&nowTime, &utcTime) == nullptr) {
		std::lock_guard<std::mutex> lock(clockMutex);
		clockText.clear();
		errorCode = 1;
		return;
	}

	// CET is UTC+1 and CEST is UTC+2 during EU daylight saving period.
	const int utcOffsetHours = isCentralEuropeanSummerTimeUtc(utcTime) ? 2 : 1;
	const std::time_t centralEuropeanTime = nowTime + (utcOffsetHours * 60 * 60);

	std::tm cetTime{};
	if (gmtime_r(&centralEuropeanTime, &cetTime) == nullptr) {
		std::lock_guard<std::mutex> lock(clockMutex);
		clockText.clear();
		errorCode = 1;
		return;
	}

	std::ostringstream stream;
	stream << std::put_time(&cetTime, "%H:%M");

	std::lock_guard<std::mutex> lock(clockMutex);
	clockText = stream.str();
	errorCode = 0;
}
