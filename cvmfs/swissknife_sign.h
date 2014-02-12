/**
 * This file is part of the CernVM File System.
 */

#ifndef CVMFS_SWISSKNIFE_SIGN_H_
#define CVMFS_SWISSKNIFE_SIGN_H_

#include "swissknife.h"

namespace swissknife {

class CommandSign : public Command<CommandSign> {
 public:
  CommandSign(const std::string &param) : Command(param) {}
  ~CommandSign() { };
  static std::string GetName() { return "sign"; };
  static std::string GetDescription() {
    return "Adds a signature to the repository manifest.";
  };
  static ParameterList GetParameters() {
    ParameterList r;
    r.push_back(Parameter::Mandatory('m', "manifest file"));
    r.push_back(Parameter::Mandatory('r', "spooler definition"));
    r.push_back(Parameter::Mandatory('t', "directory for temporary files"));
    r.push_back(Parameter::Optional ('c', "x509 certificate"));
    r.push_back(Parameter::Optional ('k', "private key of the certificate"));
    r.push_back(Parameter::Optional ('s', "password for the private key"));
    r.push_back(Parameter::Optional ('n', "repository name"));
    r.push_back(Parameter::Optional ('h', "history path"));
    return r;
  }
  int Run(const ArgumentList &args);
};

}  // namespace swissknife

#endif  // CVMFS_SWISSKNIFE_SIGN_H_
