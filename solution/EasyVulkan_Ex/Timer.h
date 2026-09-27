#pragma once
#include <format>
#include <chrono>
#include <iostream>

class timer {
	using time_point = std::chrono::steady_clock::time_point;
	time_point start;
	time_point last;
	time_point present;
	void(*fnPrint)(double);
	// Static
	static constexpr auto PrintToConsole = [](double value) { std::cout << std::format("{}\n", value); };
public:
	timer(void(*fnPrint)(double) = PrintToConsole) :
		fnPrint(fnPrint) {
		start = last = std::chrono::steady_clock::now();
	}
	~timer() { if (fnPrint) fnPrint(DurationFromStart()); }
	// Setter
	void FnPrint(void(*fnPrint)(double)) { this->fnPrint = fnPrint; }
	// Non-const Function
	void TickTock() {
		last = present = std::chrono::steady_clock::now();
	}
	double DurationFromStart() {
		TickTock();
		return std::chrono::duration<double>(present - start).count();
	}
	double DurationFromLast() {
		present = std::chrono::steady_clock::now();
		double duration = std::chrono::duration<double>(present - last).count();
		last = present;
		return duration;
	}
	operator double() { return DurationFromLast(); }
	void Print() { fnPrint(DurationFromLast()); }
};
class durations {
	std::vector<std::pair<double, const char*>> valueAndDescriptionPairs;
	timer timer = { nullptr };
	// Static
	static constexpr auto PrintToConsole = [](double value, const char* description) { std::cout << std::format("{}: {}\n", description, value); };
public:
	durations() = default;
	// Getter
	double operator[](size_t index) const { return valueAndDescriptionPairs[index].first; }
	const char* Description(size_t index) const { return valueAndDescriptionPairs[index].second; }
	size_t Count() const { return valueAndDescriptionPairs.size(); }
	// Const Function
	double Sum() const {
		double sum = 0;
		for (auto& i : valueAndDescriptionPairs)
			sum += i.first;
		return sum;
	}
	void Print(void(*fnPrint)(double, const char*) = PrintToConsole) const {
		for (auto& i : valueAndDescriptionPairs)
			fnPrint(i.first, i.second);
	}
	// Non-const Function
	void TickTock() {
		timer.TickTock();
	}
	void SaveDurationFromLast(const char* description = "") {
		valueAndDescriptionPairs.emplace_back(timer.DurationFromLast(), description);
		TickTock();
	}
};