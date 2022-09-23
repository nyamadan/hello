#ifndef __ARGUMENTS_HPP__
#define __ARGUMENTS_HPP__

#include <iostream>
#include <string>

namespace hello::utils::arguments {

class ParsedArguments {
public:
  std::string file;
  ParsedArguments(const std::string &file) { this->file = file; }
};

class ShowHelp : public std::runtime_error {
public:
  std::string problem;
  ShowHelp(const std::string &problem) : std::runtime_error(problem) {}
  virtual ~ShowHelp() {}
};

class ArgumentsError : public std::runtime_error {
  std::string problem;

public:
  ArgumentsError(const std::string &problem) : std::runtime_error(problem) {}
  virtual ~ArgumentsError() {}
};

ParsedArguments parse(const int32_t argc, const char *const *argv,
                      std::ostream &cout, std::ostream &cerr);
} // namespace hello::utils::arguments
#endif