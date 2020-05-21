#include "cxxopts.hpp"
#include <string>


using std::string;

cxxopts::ParseResult parse(int argc, char* argv[]) {
  cxxopts::Options options(argv[0]);
  try {

    //options.positional_help("[-s | -c <host>] [options]").show_positional_help();
    options.custom_help("[-s | -c <host>] [options]");

    options.add_options()
      ("s,server", "run in server mode", cxxopts::value<bool>())
      ("c,client", "run in client mode, connecting to <host>", cxxopts::value<string>(),"<host>")
      ("p,port", "server port to listen on/connect to", cxxopts::value<int>()->default_value("43210"), "<port>")
      ("b,bandwidth", "target bandwidth in Kbits/sec", cxxopts::value<int>())
      ("i,interval", "seconds between periodic bandwidth reports", cxxopts::value<int>()->default_value("1"), "<sec>")
      ("t,time", "time in seconds to transmit for", cxxopts::value<int>()->default_value("10"), "<sec>")
      ("h,help", "show this help");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    if (!result.count("s") && !result.count("c")) { 
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    return result;

  } catch (const cxxopts::OptionException& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    std::cout << options.help({""}) << std::endl;
    exit(1);
  }
}