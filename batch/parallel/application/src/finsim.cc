#include <fstream>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <random>
#include <string>
#include <thread>
#include <vector>

struct InputConfig {
  // Total simulations (e.g. 10 simulations).
  std::int64_t simulations;
  // Total iterations per simulation in days (e.g. 100 days).
  std::int64_t iterations;
  // Stock ticker name (e.g. GOOG).
  std::string ticker;
  // Initial stock price (e.g. $100).
  double price;
  // Daily percent variance (e.g. 5%)
  double variance;
  // Standard deviation in variance (e.g. 5% +/- 1%).
  double deviation;
  // Total threads used (max available using hardware).
  std::int64_t total_threads;
};

void print_input_config(InputConfig const& config) {
  std::cout << std::fixed << std::setprecision(2);

  std::cout << "\nInput Configuration\n";
  std::cout << "---------------------\n";
  std::cout << "Simulations:  " << config.simulations << "\n";
  std::cout << "Iterations:   " << config.iterations << "\n";
  std::cout << "Ticker:       " << config.ticker << "\n";
  std::cout << "Price:        $" << config.price << "\n";
  std::cout << "Variance:     " << config.variance * 100.0 << " +/- "
            << config.deviation * 100.0 << "%\n";
  std::cout << "Threads:      " << config.total_threads << "\n";
}

double simulate(InputConfig const& config) {
  double updated_price = config.price;
  thread_local static std::mt19937 generator(std::random_device{}());
  std::uniform_real_distribution<> dis(-1 * config.deviation, config.deviation);

  for (int j = 0; j < config.iterations; ++j) {
    updated_price =
        updated_price * (1 + ((config.variance + dis(generator)) * 0.01));
  }
  return updated_price;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "usage: finsim <input>\n";
    return 1;
  }

  std::string input_filename = std::string(argv[1]);

  std::ifstream input_file;
  input_file.open(input_filename);
  std::vector<std::string> lines;
  if (input_file.is_open()) {
    std::string line;
    while (std::getline(input_file, line)) {
      lines.push_back(line);
    }
  } else {
    std::cout << "Couldn't open file\n";
    return 1;
  }

  if (lines.size() != 6) {
    std::cout << "Input is not the expected length\n";
    return 1;
  }

  InputConfig input_config;
  input_config.simulations = stoi(lines.at(0));
  input_config.iterations = stoi(lines.at(1));
  input_config.ticker = lines.at(2);
  input_config.price = stof(lines.at(3));
  input_config.variance = stof(lines.at(4));
  input_config.deviation = stof(lines.at(5));

  auto total_threads =
      static_cast<std::int64_t>(std::thread::hardware_concurrency());
  input_config.total_threads = total_threads;

  print_input_config(input_config);

  std::vector<double> results(input_config.simulations);
  simulate(input_config);

#pragma omp parallel for
  for (int i = 0; i < input_config.simulations; i++) {
    results[i] = simulate(input_config);
  }

  double sum = 0.0;
#pragma omp parallel for reduction(+ : sum)
  for (int i = 0; i < input_config.simulations; i++) {
    sum += results[i];
  }
  double mean = sum / input_config.simulations;

  std::cout << "\nOutput\n";
  std::cout << "---------------------\n";
  std::cout << "Mean final price: " << mean << "\n";

  return 0;
}
