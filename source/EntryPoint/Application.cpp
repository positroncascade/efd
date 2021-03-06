#include "Application.hpp"

#include "CsvReporter.hpp"
#include "Simulation/SimulationController.hpp"
#include "Simulation/facade.hpp"
#include "XmlConfigurationParser.hpp"

#include <precice/SolverInterface.hpp>

#include <Uni/Firewall/Interface>
#include <Uni/Logging/macros>
#include <Uni/executablepath>
#include <Uni/pathoperations>

#include <petscsys.h>

#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <memory>

//

using Fluid::EntryPoint::Application;

class Fluid::EntryPoint::ApplicationPrivateImplementation {
  using PreciceInterface = precice::SolverInterface;

  using UniquePreciceInterface = std::unique_ptr<PreciceInterface>;

  using UniqueSimulationController
          = std::unique_ptr<Simulation::SimulationController>;

  using Path = boost::filesystem::path;

  ApplicationPrivateImplementation(Application* in,
                                   int const&   argc_,
                                   char**       argv_) : _in(in),
    argc(argc_),
    argv(argv_),
    masterRank(0),
    fluidConfiguration(new Simulation::Configuration()) {
    namespace fs          = boost::filesystem;
    using LocaleGenerator = boost::locale::generator;
    globalLocale          = LocaleGenerator().generate("");
    std::locale::global(globalLocale);
    Path::imbue(std::locale());

    LocaleGenerator ansiLocaleGenerator;
    ansiLocaleGenerator.use_ansi_encoding(true);
    ansiLocale = ansiLocaleGenerator.generate("");

    applicationPath = Path(Uni::getExecutablePath()).parent_path();

    outputDirectoryPath = fs::current_path();
  }

  Uni_Firewall_INTERFACE_LINK(Application)

  int argc;
  char**      argv;
  std::locale globalLocale;
  std::locale ansiLocale;
  Path        applicationPath;

  Path        outputDirectoryPath;
  Path        preciceConfigurationPath;
  Path        fluidConfigurationPath;
  Path        petscConfigurationPath;
  std::string outputFileNamePrefix;

  int masterRank;
  int rank;
  int processCount;

  std::unique_ptr<Simulation::Configuration> fluidConfiguration;

  UniquePreciceInterface     preciceInterface;
  UniqueSimulationController simulationController;

  std::unique_ptr<Simulation::Reporter> reporter;

  bool
  isMaster() {
    return rank == masterRank;
  }

  void
  initializeReporter() {
    if (isMaster()) {
      reporter.reset(new CsvReporter());
    } else {
      reporter.reset(new Simulation::Reporter());
    }
    reporter->initialize(outputDirectoryPath,
                         outputFileNamePrefix);
  }
};

Application::
Application(int const& argc, char** argv)
  : _im(new Implementation(this,
                           argc,
                           argv)) {}

Application::
~Application() {}

int
Application::
parseArguments() {
  namespace po = boost::program_options;

  po::options_description optionDescription("Fluid Solver Arguments");
  optionDescription.add_options()
    ("debug,d",
    "enable extra debug output from the fluid solver")
    ("help,h", "produce a help message and leave the program")
    ("no-immersed-boundary,n",
    "disable immersed-boundary computation, i.e. disable PreCICE usage")
    ("output-directory,o",
    po::value<std::string>()->default_value("output"),
    "output directory path")
    ("petsc,e",
    po::value<std::string>()->default_value("petsc-config.conf"),
    "PETSc configuration path")
    ("precice,p",
    po::value<std::string>()->default_value("precice-config.xml"),
    "PreCICE configuration path")
    ("simulation,s",
    po::value<std::string>()->default_value("fluid-config.xml"),
    "fluid solver simulation configuration path");

  po::variables_map options;
  po::store(po::parse_command_line(_im->argc, _im->argv, optionDescription),
            options);
  po::notify(options);

  if (options.count("help")) {
    std::cout << optionDescription << "\n";

    return 1;
  }

  if (options.count("output-directory")) {
    _im->outputDirectoryPath = options["output-directory"].as<std::string>();
    boost::filesystem::create_directories(_im->outputDirectoryPath);
    _im->outputDirectoryPath = boost::filesystem::canonical(
      options["output-directory"].as<std::string>());
  }

  if (options.count("petsc")) {
    _im->petscConfigurationPath = boost::filesystem::canonical(
      options["petsc"].as<std::string>());
  }

  if (options.count("precice")) {
    _im->fluidConfiguration->doImmersedBoundary = true;

    _im->preciceConfigurationPath
      = boost::filesystem::absolute(options["precice"].as<std::string>());
  }

  if (options.count("no-immersed-boundary")) {
    _im->fluidConfiguration->doImmersedBoundary = false;
  } else {
    _im->fluidConfiguration->doImmersedBoundary = true;
  }

  if (options.count("simulation")) {
    _im->fluidConfigurationPath = boost::filesystem::canonical(
      options["simulation"].as<std::string>());
  } else {
    throwException("No configuration was provided, try --help, -h");
  }

  if (options.count("debug")) {
    _im->fluidConfiguration->doDebug = true;
  } else {
    _im->fluidConfiguration->doDebug = false;
  }

  return 0;
}

void
Application::
initialize() {
  auto petscConfigurationPath
    = Uni::convertUtfPathToAnsi(
    boost::filesystem::make_relative(
      _im->petscConfigurationPath).string(),
    _im->globalLocale,
    _im->ansiLocale);

  PetscInitialize(&_im->argc, &_im->argv,
                  petscConfigurationPath.c_str(),
                  PETSC_NULL);
  MPI_Comm_size(PETSC_COMM_WORLD, &_im->processCount);
  MPI_Comm_rank(PETSC_COMM_WORLD, &_im->rank);

  parseSimulationConfiguration();
  createOutputDirectory();
  initializePrecice();

  _im->simulationController
    = Simulation::create_simulation_controller(
    _im->fluidConfiguration.get());

  _im->initializeReporter();

  auto sendWallToReporter
    = [&] (unsigned t, unsigned u) {
        auto wallEnumToInt
          = [&] (Simulation::WallEnum en) {
              return en
                     == Simulation::WallEnum::Input ? 0 : en
                     == Simulation::WallEnum::ParabolicInput ? 1 : en
                     == Simulation::WallEnum::Output ? 2
                     : -1;
            };

        std::string prefix = std::to_string(t) + std::to_string(u);

        _im->reporter->setAt(
          prefix + "WallType",
          wallEnumToInt(_im->fluidConfiguration->walls[t][u]->type()));

        if (_im->fluidConfiguration->dimensions == 2) {
          Eigen::Matrix<double, 2, 1> velocity;
          velocity << _im->fluidConfiguration->walls[t][u]
            ->velocity().cast<double>().head<2>();
          _im->reporter->setAt(prefix + "WallVelocity", velocity);
        } else {
          Eigen::Matrix<double, 3, 1> velocity;
          velocity << _im->fluidConfiguration->walls[t][u]
            ->velocity().cast<double>().head<3>();
          _im->reporter->setAt(prefix + "WallVelocity", velocity);
        }
      };

  sendWallToReporter(0, 0);
  sendWallToReporter(0, 1);
  sendWallToReporter(1, 0);
  sendWallToReporter(1, 1);

  if (_im->fluidConfiguration->dimensions == 3) {
    sendWallToReporter(2, 0);
    sendWallToReporter(2, 1);
  }

  _im->simulationController->initialize(_im->preciceInterface.get(),
                                        _im->reporter.get(),
                                        _im->outputDirectoryPath,
                                        _im->outputFileNamePrefix);
}

void
Application::
run() {
  namespace fs = boost::filesystem;

  auto report_template_path
    = _im->applicationPath / "Report";

  bool do_html_report = false;

  if (fs::is_directory(report_template_path)) {
    do_html_report = true;
  }

  if (do_html_report) {
    fs::copy_file(
      report_template_path / "Template.html",
      _im->outputDirectoryPath / (_im->outputFileNamePrefix + ".html"),
      fs::copy_option::overwrite_if_exists);

    fs::create_directory(_im->outputDirectoryPath / "js");
    auto it     = fs::recursive_directory_iterator(report_template_path / "js");
    auto end_it = fs::recursive_directory_iterator();

    for (; it != end_it; ++it) {
      if (it->status().type() == fs::file_type::regular_file) {
        fs::copy_file(
          it->path(),
          _im->outputDirectoryPath / "js" / it->path().filename(),
          fs::copy_option::overwrite_if_exists);
      }
    }
  }

  _im->reporter->recordInfo();

  while (true) {
    if (_im->preciceInterface) {
      if (!_im->preciceInterface->isCouplingOngoing()) {
        break;
      }
    }

    if (!_im->simulationController->iterate()) {
      break;
    }

    if (_im->isMaster()) {
      logInfo("N = {1}; t = {2}",
              _im->simulationController->iterationNumber(),
              _im->simulationController->time());
    }

    _im->reporter->recordIteration();
  }

  if (_im->preciceInterface) {
    _im->preciceInterface->finalize();
  }

  _im->reporter->release();
}

void
Application::
release() {
  PetscFinalize();
  _im->fluidConfiguration->reset();
}

void
Application::
parseSimulationConfiguration() {
  XmlConfigurationParser(_im->fluidConfiguration,
                         _im->fluidConfigurationPath);
  logInfo(_im->fluidConfiguration->toString());
}

void
Application::
createOutputDirectory() {
  Implementation::Path outputDirectoryPath(_im->fluidConfiguration->filename);

  if (outputDirectoryPath.is_relative()) {
    outputDirectoryPath = (_im->outputDirectoryPath / outputDirectoryPath);
  }

  auto outputFileName = outputDirectoryPath.filename();

  outputDirectoryPath = outputDirectoryPath.parent_path();
  boost::filesystem::create_directories(outputDirectoryPath);

  _im->outputDirectoryPath  = outputDirectoryPath;
  _im->outputFileNamePrefix = outputFileName.string();
}

void
Application::
initializePrecice() {
  namespace fs = boost::filesystem;
  using namespace precice;

  if (!_im->fluidConfiguration->doImmersedBoundary) {
    return;
  }

  bool     has_path = false;
  fs::path config_path;

  if (!fs::is_regular_file(_im->preciceConfigurationPath)
      && !fs::is_symlink(_im->preciceConfigurationPath)) {
    try {
      config_path
        = _im->fluidConfiguration->tryToGet<boost::filesystem::path>(
        "/Ib/PreciceConfigurationPath");
      has_path = true;
    } catch (std::exception const&) {}

    if (!has_path) {
      return;
    }
    config_path = boost::filesystem::canonical(config_path);
  } else {
    config_path = _im->preciceConfigurationPath;
  }

  // Change current working directory of the application to overcome a Precice
  // configuration issue with python modules paths.
  // boost::filesystem::current_path(config_path.parent_path());

  auto preciceConfigurationPath
    = Uni::convertUtfPathToAnsi(
    boost::filesystem::make_relative(config_path).string(),
    _im->globalLocale, _im->ansiLocale);

  _im->preciceInterface.reset(
    new Implementation::PreciceInterface("Fluid",
                                         _im->rank,
                                         _im->processCount));
  _im->preciceInterface->configure(preciceConfigurationPath);
}
