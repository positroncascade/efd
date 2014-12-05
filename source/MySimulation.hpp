#ifndef FsiSimulation_MySimulation_hpp
#define FsiSimulation_MySimulation_hpp

#include <boost/filesystem.hpp>

namespace FsiSimulation {
class MySimulation {
public:
  typedef boost::filesystem::path Path;

public:
  virtual
  ~MySimulation() {}

  virtual void
  initialize(Path const&        outputDirectory,
             std::string const& fileNamePrefix) = 0;

  virtual bool
  iterate() = 0;
};
}
#endif
