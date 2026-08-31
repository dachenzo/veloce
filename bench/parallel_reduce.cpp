#include <veloce/parallel_reduce.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <span>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using Microseconds = std::chrono::duration<double, std::micro>;

constexpr std::size_t default_element_count = 50'000'000;
constexpr std::size_t default_iterations = 9;
constexpr std::size_t max_thread_count = 4;

double median(std::vector<double> samples) {
	std::ranges::sort(samples);
	return samples[samples.size() / 2];
}

} // namespace

int main() {
	const std::size_t element_count = default_element_count;
	const std::size_t iterations = default_iterations;
	const std::size_t thread_count = max_thread_count;

	std::vector<std::uint64_t> input(element_count);
	std::iota(input.begin(), input.end(), std::uint64_t{1});

	veloce::ThreadPool pool(thread_count);
	veloce::ParallelReduce parallel_reduce(pool);
	std::vector<double> serial_samples;
	std::vector<double> parallel_samples;
	serial_samples.reserve(iterations);
	parallel_samples.reserve(iterations);

	std::uint64_t checksum = 0;
	for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
		std::uint64_t serial_result = 0;
		std::uint64_t parallel_result = 0;

		const auto run_serial = [&] {
			const auto start = Clock::now();
			serial_result = std::accumulate(
				input.begin(), input.end(), std::uint64_t{0});
			return Microseconds{Clock::now() - start}.count();
		};
		const auto run_parallel = [&] {
			const auto start = Clock::now();
			parallel_result = parallel_reduce.operator()<
				std::uint64_t,
				std::dynamic_extent,
				std::plus<std::uint64_t>,
				veloce::ReduceOrder::LeftRight>(
					std::span{input}, std::uint64_t{0}, std::plus<std::uint64_t>{});
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

		if (serial_result != parallel_result) {
			std::cerr << "Serial and parallel results differ\n";
			return 1;
		}

		checksum ^= parallel_result;
		serial_samples.push_back(serial_time);
		parallel_samples.push_back(parallel_time);
	}

	const double serial_median = median(serial_samples);
	const double parallel_median = median(parallel_samples);

	std::cout << "Elements:        " << element_count << '\n'
			  << "Iterations:      " << iterations << '\n'
			  << "Worker threads:  " << thread_count << '\n'
			  << "Serial median:   " << serial_median / 1000.0 << " ms\n"
			  << "Parallel median: " << parallel_median / 1000.0 << " ms\n"
			  << "Speedup:         " << serial_median / parallel_median << "x\n"
			  << "Checksum:        " << checksum << '\n';
}