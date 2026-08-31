#include <veloce/parallel.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <span>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using Microseconds = std::chrono::duration<double, std::micro>;

constexpr std::size_t default_element_count = 50'000'000;
constexpr std::size_t default_iterations = 9;
constexpr std::size_t max_thread_count = 4; // only have four cores :(


void transform(std::uint64_t& value) {
	for (int round = 0; round < 32; ++round) {
		value ^= value >> 12;
		value ^= value << 25;
		value ^= value >> 27;
		value *= 0x2545F4914F6CDD1DULL;
	}
}

double median(std::vector<double> samples) {
	std::ranges::sort(samples);
	return samples[samples.size() / 2];
}

} // namespace

int main( ) {
	const std::size_t element_count = default_element_count;
	const std::size_t iterations = default_iterations;
	const std::size_t thread_count = max_thread_count;


	std::vector<std::uint64_t> input(element_count);
	std::iota(input.begin(), input.end(), std::uint64_t{1});

	veloce::ThreadPool pool(thread_count);
	veloce::Parallel parallel(pool);
	std::vector<double> serial_samples;
	std::vector<double> parallel_samples;
	serial_samples.reserve(iterations);
	parallel_samples.reserve(iterations);

	std::uint64_t checksum = 0;
	const auto operation = [](std::uint64_t& value) { transform(value); };

	for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
		auto serial_data = input;
		auto parallel_data = input;

		const auto run_serial = [&] {
			const auto start = Clock::now();
			std::ranges::for_each(serial_data, operation);
			return Microseconds{Clock::now() - start}.count();
		};
		const auto run_parallel = [&] {
			const auto start = Clock::now();
			parallel(std::span{parallel_data}, operation);
			return Microseconds{Clock::now() - start}.count();
		};

		double serial_time = 0.0;
		double parallel_time = 0.0;
		if (iteration % 2 == 0) {
			serial_time = run_serial();
			parallel_time = run_parallel();
		} else {
			parallel_time = run_parallel();
			serial_time = run_serial();
		}

		if (serial_data != parallel_data) {
			std::cerr << "Serial and parallel results differ\n";
			return 1;
		}

		checksum ^= std::accumulate(
			parallel_data.begin(), parallel_data.end(), std::uint64_t{0});
		serial_samples.push_back(serial_time);
		parallel_samples.push_back(parallel_time);
	}

	const double serial_median = median(serial_samples);
	const double parallel_median = median(parallel_samples);

	std::cout << "Elements:        " << element_count << '\n'
			  << "Iterations:      " << iterations << '\n'
			  << "Worker threads:  " << thread_count;
	
	std::cout << '\n'
			  << "Serial median:   " << serial_median / 1000.0 << " ms\n"
			  << "Parallel median: " << parallel_median / 1000.0 << " ms\n"
			  << "Speedup:         " << serial_median / parallel_median << "x\n"
			  << "Checksum:        " << checksum << '\n';
}
