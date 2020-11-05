// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// [START cloudrun_helloworld_service]
// [START cloud_run_hello_world]
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <thread>

namespace be = boost::beast;
namespace asio = boost::asio;
namespace po = boost::program_options;
using tcp = boost::asio::ip::tcp;

po::variables_map parse_args(int& argc, char* argv[]) {
  // Initialize the default port with the value from the "PORT" environment
  // variable or with 8080.
  auto port = [&]() -> std::uint16_t {
    auto env = std::getenv("PORT");
    if (env == nullptr) return 8080;
    auto value = std::stoi(env);
    if (value < std::numeric_limits<std::uint16_t>::min() ||
        value > std::numeric_limits<std::uint16_t>::max()) {
      std::ostringstream os;
      os << "The PORT environment variable value (" << value
         << ") is out of range.";
      throw std::invalid_argument(std::move(os).str());
    }
    return static_cast<std::uint16_t>(value);
  }();

  // Parse the command-line options.
  po::options_description desc("Server configuration");
  desc.add_options()
      //
      ("help", "produce help message")
      //
      ("address", po::value<std::string>()->default_value("0.0.0.0"),
       "set listening address")
      //
      ("port", po::value<std::uint16_t>()->default_value(port),
       "set listening port");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << "\n";
  }
  return vm;
}

int main(int argc, char* argv[]) try {
  po::variables_map vm = parse_args(argc, argv);

  if (vm.count("help")) return 0;

  auto address = asio::ip::make_address(vm["address"].as<std::string>());
  auto port = vm["port"].as<std::uint16_t>();
  std::cout << "Listening on " << address << ":" << port << std::endl;

  auto handle_session = [](tcp::socket socket) {
    auto report_error = [](be::error_code ec, char const* what) {
      std::cerr << what << ": " << ec.message() << "\n";
    };

    be::error_code ec;
    for (;;) {
      be::flat_buffer buffer;

      // Read a request
      be::http::request<be::http::string_body> request;
      be::http::read(socket, buffer, request, ec);
      if (ec == be::http::error::end_of_stream) break;
      if (ec) return report_error(ec, "read");

      // Send the response
      // Respond to any request with a "Hello World" message.
      be::http::response<be::http::string_body> response{be::http::status::ok,
                                                         request.version()};
      response.set(be::http::field::server, BOOST_BEAST_VERSION_STRING);
      response.set(be::http::field::content_type, "text/plain");
      response.keep_alive(request.keep_alive());
      std::string greeting = "Hello ";
      auto const* target = std::getenv("TARGET");
      greeting += target == nullptr ? "World" : target;
      greeting += "\n";
      response.body() = std::move(greeting);
      response.prepare_payload();
      be::http::write(socket, response, ec);
      if (ec) return report_error(ec, "write");
    }
    socket.shutdown(tcp::socket::shutdown_send, ec);
  };

  asio::io_context ioc{/*concurrency_hint=*/1};
  tcp::acceptor acceptor{ioc, {address, port}};
  for (;;) {
    auto socket = acceptor.accept(ioc);
    if (!socket.is_open()) break;
    // Run a thread per-session, transferring ownership of the socket
    std::thread{handle_session, std::move(socket)}.detach();
  }

  return 0;
} catch (std::exception const& ex) {
  std::cerr << "Standard exception caught " << ex.what() << '\n';
  return 1;
}
// [END cloud_run_hello_world]
// [END cloudrun_helloworld_service]
