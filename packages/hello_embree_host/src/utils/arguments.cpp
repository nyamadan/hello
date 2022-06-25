#include "./arguments.hpp"

#include <args.hxx>
#include <iostream>
#include <string>

namespace hello::utils::arguments {
ParsedArguments parse(const int32_t argc, const char *const *argv,
                      std::ostream &cout, std::ostream &cerr) {
  args::ArgumentParser parser("This is a test program.");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::ValueFlag<std::string> file(parser, "file", "Input file flag",
                                    {'f', "file"}, args::Options::Required);

  try {
    parser.ParseCLI(argc, argv);
  } catch (const args::Help &h) {
    cout << parser;
    throw ShowHelp(h.what());
  } catch (const args::RequiredError &e) {
    cerr << e.what() << std::endl;
    cerr << parser;
    throw ArgumentsError(e.what());
  } catch (const args::ParseError &e) {
    cerr << e.what() << std::endl;
    cerr << parser;
    throw ArgumentsError(e.what());
  }

  return ParsedArguments(args::get(file));
}
} // namespace hello::utils::arguments
