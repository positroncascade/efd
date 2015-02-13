#ifndef FsiSimulation_EntryPoint_SimulationBuilder_hpp
#define FsiSimulation_EntryPoint_SimulationBuilder_hpp

#include "FluidSimulation/Cell.hpp"
#include "FluidSimulation/Configuration.hpp"
#include "FluidSimulation/FdSimulation.hpp"
#include "FluidSimulation/GhostLayer/InitializationActions.hpp"
#include "FluidSimulation/GhostLayer/PetscExchangeActions.hpp"
#include "FluidSimulation/GridGeometry.hpp"

#include "StructuredMemory/Accessor.hpp"
#include "StructuredMemory/Memory.hpp"

#include <Uni/StructuredGrid/Basic/MultiIndex>

namespace FsiSimulation {
namespace EntryPoint {
template <typename Scalar, int TD>
class SimulationBuilder {
public:
  typedef FluidSimulation::FdSimulation<
      FluidSimulation::UniformGridGeometry<Scalar, TD>,
      StructuredMemory::IterableMemory<FluidSimulation::Cell<Scalar, TD>, TD>,
      Scalar,
      TD> Simulation;
  typedef typename Simulation::GridType                 GridS;
  typedef typename Simulation::GridGeometryType         GridGeometryS;
  typedef typename Simulation::ParallelDistributionType ParallelTopologyS;
  typedef typename Simulation::GhostHandlersType        GhostCellsHandlerS;
  typedef typename Simulation::MemoryType               MemoryS;
  typedef typename Simulation::CellAccessorFactory
    CellAccessorFactoryS;
  typedef typename GridS::CellAccessorType CellAccessorS;
  typedef typename CellAccessorS::CellType CellS;
  typedef typename CellS::Velocity         VelocityS;
  typedef typename CellS::Pressure         PressureS;

  template <int TDimension, int TDirection>
  struct GhostHandlers {
    typedef
      FluidSimulation::GhostLayer::MpiExchange::Handler
      <Scalar, typename GridS::Base,
       SimulationBuilder::template fghAccessor<TDimension>,
       Scalar, TD, TDimension, TDirection>
      FghMpiExchange;
    typedef
      FluidSimulation::GhostLayer::MpiExchange::Handler
      <PressureS, typename GridS::Base,
       SimulationBuilder::pressureAccessor, Scalar, TD,
       TDimension, TDirection>
      PressureMpiExchange;
    typedef
      FluidSimulation::GhostLayer::MpiExchange::Handler
      <VelocityS, typename GridS::Base,
       SimulationBuilder::velocityAccessor, Scalar, TD,
       TDimension, TDirection>
      VelocityMpiExchange;

    typedef
      FluidSimulation::GhostLayer::Initialization::MovingWallFghAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      MovingWallFghInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, MovingWallFghInitializationAction, TD,
       TDimension, TDirection>
      MovingWallFghInitialization;
    typedef
      FluidSimulation::GhostLayer::Initialization::InputFghAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      InputFghInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, InputFghInitializationAction, TD,
       TDimension, TDirection>
      InputFghInitialization;
    typedef
      FluidSimulation::GhostLayer::Initialization::ParabolicInputFghAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      ParabolicInputFghInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, ParabolicInputFghInitializationAction, TD,
       TDimension, TDirection>
      ParabolicInputFghInitialization;
    typedef
      FluidSimulation::GhostLayer::Initialization::OutputFghAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      OutputFghInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, OutputFghInitializationAction, TD,
       TDimension, TDirection>
      OutputFghInitialization;

    typedef
      FluidSimulation::GhostLayer::LsStencilGenerator::Handler
      <GridS, TDimension, TDirection>
      PpeStencilGenerationHandler;

    typedef
      FluidSimulation::GhostLayer::PetscExchange::ConstantRhsGenerationAction<
        CellAccessorS>
      PpeRhsGenerationAction;
    typedef
      FluidSimulation::GhostLayer::PetscExchange::Handler
      <GridS, PpeRhsGenerationAction, TDimension, TDirection>
      PpeRhsGenerationHandler;

    typedef
      FluidSimulation::GhostLayer::PetscExchange::PpeRhsAcquiererAction
      PpeRhsAcquiererAction;
    typedef
      FluidSimulation::GhostLayer::PetscExchange::Handler
      <GridS, PpeRhsAcquiererAction, TDimension, TDirection>
      PpeRhsAcquiererHandler;

    typedef
      FluidSimulation::GhostLayer::Initialization::MovingWallVelocityAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      MovingWallVelocityInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, MovingWallVelocityInitializationAction, TD,
       TDimension, TDirection>
      MovingWallVelocityInitialization;
    typedef
      FluidSimulation::GhostLayer::Initialization::InputVelocityAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      InputVelocityInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, InputVelocityInitializationAction, TD,
       TDimension, TDirection>
      InputVelocityInitialization;
    typedef
      FluidSimulation::GhostLayer::Initialization::ParabolicInputVelocityAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      ParabolicInputVelocityInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, ParabolicInputVelocityInitializationAction, TD,
       TDimension, TDirection>
      ParabolicInputVelocityInitialization;
    typedef
      FluidSimulation::GhostLayer::Initialization::OutputVelocityAction
      <typename GridS::Base, Scalar, TD, TDimension, TDirection>
      OutputVelocityInitializationAction;
    typedef
      FluidSimulation::GhostLayer::Initialization::Handler
      <typename GridS::Base, OutputVelocityInitializationAction, TD,
       TDimension, TDirection>
      OutputVelocityInitialization;
  };

public:
  SimulationBuilder(FluidSimulation::Configuration* configuration)
    : _configuration(configuration) {
    _simulation           = new Simulation();
    _grid                 = &_simulation->_grid;
    _parallelDistribution = &_simulation->_parallelDistribution;
    _handlers             = &_simulation->_ghostHandler;
    _maxVelocity          = &_simulation->_maxVelocity;

    typedef typename ParallelTopologyS::VectorDi VectorDi;
    typedef typename GridGeometryS::VectorDs     VectorDs;

    VectorDi processorSize;
    VectorDi globalCellSize;
    VectorDs width;
    processorSize(0)  = configuration->parallelizationSize(0);
    globalCellSize(0) = configuration->size(0);
    width(0)          = (Scalar)configuration->width(0);
    processorSize(1)  = configuration->parallelizationSize(1);
    globalCellSize(1) = configuration->size(1);
    width(1)          = (Scalar)configuration->width(1);

    if (TD == 3) {
      processorSize(2)  = configuration->parallelizationSize(2);
      globalCellSize(2) = configuration->size(2);
      width(2)          = (Scalar)configuration->width(2);
    }

    int rank;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
    _parallelDistribution->initialize(rank,
                                      processorSize,
                                      globalCellSize);

    _simulation->_parameters.re()    = configuration->re;
    _simulation->_parameters.gamma() = configuration->gamma;
    _simulation->_parameters.tau()   = configuration->tau;
    _simulation->_parameters.alpha() = configuration->alpha;
    _simulation->_parameters.immersedBoundaryMethod()
                                  = configuration->immersedBoundaryMethod;
    _simulation->_parameters.g(0) = configuration->environment(0);
    _simulation->_parameters.g(1) = configuration->environment(1);
    _simulation->_iterationLimit  = configuration->iterationLimit;
    _simulation->_timeLimit       = configuration->timeLimit;
    _simulation->_plotInterval    = configuration->plotInterval;

    if (TD == 3) {
      _simulation->_parameters.g(2) = configuration->environment(2);
    }

    VectorDi localSize(_parallelDistribution->localCellSize + 2 *
                       VectorDi::Ones());

    _simulation->setParameters(localSize, width);
  }

  Simulation*
  simulation() const {
    return _simulation;
  }

  void
  setLeftAsMoving() {
    if (_parallelDistribution->neighbors[0][0] < 0) {
      _setLeftAsMoving();
    } else {
      _setLeftAsMpiExchange();
    }
  }

  void
  setLeftAsInput() {
    if (_parallelDistribution->neighbors[0][0] < 0) {
      _setLeftAsInput();
    } else {
      _setLeftAsMpiExchange();
    }
  }

  void
  setLeftAsParabolicInput() {
    if (_parallelDistribution->neighbors[0][0] < 0) {
      _setLeftAsParabolicInput();
    } else {
      _setLeftAsMpiExchange();
    }
  }

  void
  setLeftAsOutput() {
    if (_parallelDistribution->neighbors[0][0] < 0) {
      _setLeftAsOutput();
    } else {
      _setLeftAsMpiExchange();
    }
  }

  void
  setRightAsMoving() {
    if (_parallelDistribution->neighbors[0][1] < 0) {
      _setRightAsMoving();
    } else {
      _setRightAsMpiExchange();
    }
  }

  void
  setRightAsInput() {
    if (_parallelDistribution->neighbors[0][1] < 0) {
      _setRightAsInput();
    } else {
      _setRightAsMpiExchange();
    }
  }

  void
  setRightAsOutput() {
    if (_parallelDistribution->neighbors[0][1] < 0) {
      _setRightAsOutput();
    } else {
      _setRightAsMpiExchange();
    }
  }

  void
  setBottomAsMoving() {
    if (_parallelDistribution->neighbors[1][0] < 0) {
      _setBottomAsMoving();
    } else {
      _setBottomAsMpiExchange();
    }
  }

  void
  setTopAsMoving() {
    if (_parallelDistribution->neighbors[1][1] < 0) {
      _setTopAsMoving();
    } else {
      _setTopAsMpiExchange();
    }
  }

  void
  setBackAsMoving() {
    if (_parallelDistribution->neighbors[2][0] < 0) {
      _setBackAsMoving();
    } else {
      _setBackAsMpiExchange();
    }
  }

  void
  setFrontAsMoving() {
    if (_parallelDistribution->neighbors[2][1] < 0) {
      _setFrontAsMoving();
    } else {
      setFrontAsMpiExchange();
    }
  }

private:
  void
  _setLeftAsMoving() {
    typedef GhostHandlers<0, 0> LeftHandlers;
    typedef typename LeftHandlers::MovingWallFghInitializationAction
      FghAction;
    typedef typename LeftHandlers::MovingWallFghInitialization
      FghHandler;

    typedef typename LeftHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename LeftHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename LeftHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;

    typedef typename LeftHandlers::MovingWallVelocityInitializationAction
      VelocityAction;
    typedef typename LeftHandlers::MovingWallVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[0][0] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[0][0],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[0][0] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[0][0] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));

    _handlers->velocityInitialization[0][0] =
      VelocityHandler::getHandler(
        &_grid->boundaries[0][0],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  _setLeftAsInput() {
    typedef GhostHandlers<0, 0> LeftHandlers;
    typedef typename LeftHandlers::InputFghInitializationAction
      FghAction;
    typedef typename LeftHandlers::InputFghInitialization
      FghHandler;

    typedef typename LeftHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename LeftHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename LeftHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;

    typedef typename LeftHandlers::InputVelocityInitializationAction
      VelocityAction;
    typedef typename LeftHandlers::InputVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[0][0] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[0][0],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[0][0] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[0][0] =
      PpeRhsGenerationHandler::getHandler(_grid, _parallelDistribution,
                                          new PpeRhsGenerationAction(0.0));

    _handlers->velocityInitialization[0][0] =
      VelocityHandler::getHandler(
        &_grid->boundaries[0][0],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  _setLeftAsParabolicInput() {
    typedef GhostHandlers<0, 0> LeftHandlers;
    typedef typename LeftHandlers::ParabolicInputFghInitializationAction
      FghAction;
    typedef typename LeftHandlers::ParabolicInputFghInitialization
      FghHandler;

    typedef typename LeftHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename LeftHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename LeftHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;

    typedef typename LeftHandlers::ParabolicInputVelocityInitializationAction
      VelocityAction;
    typedef typename LeftHandlers::ParabolicInputVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[0][0] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[0][0],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[0][0] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[0][0] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));

    _handlers->velocityInitialization[0][0] =
      VelocityHandler::getHandler(
        &_grid->boundaries[0][0],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  _setLeftAsOutput() {
    typedef GhostHandlers<0, 0> LeftHandlers;
    typedef typename LeftHandlers::OutputFghInitializationAction
      FghAction;
    typedef typename LeftHandlers::OutputFghInitialization
      FghHandler;

    typedef typename LeftHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename LeftHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename LeftHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;

    typedef typename LeftHandlers::OutputVelocityInitializationAction
      VelocityAction;
    typedef typename LeftHandlers::OutputVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[0][0] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[0][0],
        _parallelDistribution,
        new FghAction());

    _handlers->ppeStencilGeneratorStack[0][0] =
      PpeStencilGenerationHandler::getDirichletMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[0][0] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));

    _handlers->velocityInitialization[0][0] =
      VelocityHandler::getHandler(
        &_grid->boundaries[0][0],
        _parallelDistribution,
        new VelocityAction(_maxVelocity));
  }

  void
  _setLeftAsMpiExchange() {
    typedef GhostHandlers<0, 0>                        LeftHandlers;
    typedef typename LeftHandlers::FghMpiExchange      FghHandler;
    typedef typename LeftHandlers::PressureMpiExchange PressureHandler;
    typedef typename LeftHandlers::VelocityMpiExchange VelocityHandler;

    auto leftIndent  = _grid->innerGrid.leftIndent();
    auto rightIndent = _grid->innerGrid.rightIndent();

    _handlers->mpiFghExchangeStack[0][0] =
      FghHandler::getReceiveHandler(_grid,
                                    _parallelDistribution,
                                    leftIndent,
                                    rightIndent);

    _handlers->mpiPressureExchangeStack[0][0] =
      PressureHandler::getSendHandler(_grid,
                                      _parallelDistribution,
                                      leftIndent,
                                      rightIndent);

    rightIndent(1) = 0;
    rightIndent(2) = 0;

    _handlers->mpiVelocityExchangeStack[0][0] =
      VelocityHandler::getExchangeHandler(_grid,
                                          _parallelDistribution,
                                          leftIndent,
                                          rightIndent);
  }

  void
  _setRightAsMoving() {
    typedef GhostHandlers<0, 1> RightHandlers;
    typedef typename RightHandlers::MovingWallFghInitializationAction
      FghAction;
    typedef typename RightHandlers::MovingWallFghInitialization
      FghHandler;

    typedef typename RightHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename RightHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename RightHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;
    typedef typename RightHandlers::PpeRhsAcquiererAction
      PpeRhsAcquiererAction;
    typedef typename RightHandlers::PpeRhsAcquiererHandler
      PpeRhsAcquiererHandler;

    typedef typename RightHandlers::MovingWallVelocityInitializationAction
      VelocityAction;
    typedef typename RightHandlers::MovingWallVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[0][1] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[0][1],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[0][1] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[0][1] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));
    _handlers->ppeRhsAcquiererStack[0][1] =
      PpeRhsAcquiererHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsAcquiererAction());

    _handlers->velocityInitialization[0][1] =
      VelocityHandler::getHandler(
        &_grid->boundaries[0][1],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }
  void
  _setRightAsInput() {
    typedef GhostHandlers<0, 1> RightHandlers;
    typedef typename RightHandlers::InputFghInitializationAction
      FghAction;
    typedef typename RightHandlers::InputFghInitialization
      FghHandler;

    typedef typename RightHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename RightHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename RightHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;
    typedef typename RightHandlers::PpeRhsAcquiererAction
      PpeRhsAcquiererAction;
    typedef typename RightHandlers::PpeRhsAcquiererHandler
      PpeRhsAcquiererHandler;

    typedef typename RightHandlers::InputVelocityInitializationAction
      VelocityAction;
    typedef typename RightHandlers::InputVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[0][1] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[0][1],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[0][1] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[0][1] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));
    _handlers->ppeRhsAcquiererStack[0][1] =
      PpeRhsAcquiererHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsAcquiererAction());

    _handlers->velocityInitialization[0][1] =
      VelocityHandler::getHandler(
        &_grid->boundaries[0][1],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  _setRightAsOutput() {
    typedef GhostHandlers<0, 1> RightHandlers;
    typedef typename RightHandlers::OutputFghInitializationAction
      FghAction;
    typedef typename RightHandlers::OutputFghInitialization
      FghHandler;

    typedef typename RightHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename RightHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename RightHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;
    typedef typename RightHandlers::PpeRhsAcquiererAction
      PpeRhsAcquiererAction;
    typedef typename RightHandlers::PpeRhsAcquiererHandler
      PpeRhsAcquiererHandler;

    typedef typename RightHandlers::OutputVelocityInitializationAction
      VelocityAction;
    typedef typename RightHandlers::OutputVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[0][1] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[0][1],
        _parallelDistribution,
        new FghAction());

    _handlers->ppeStencilGeneratorStack[0][1] =
      PpeStencilGenerationHandler::getDirichletMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[0][1] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));
    _handlers->ppeRhsAcquiererStack[0][1] =
      PpeRhsAcquiererHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsAcquiererAction());

    _handlers->velocityInitialization[0][1] =
      VelocityHandler::getHandler(
        &_grid->boundaries[0][1],
        _parallelDistribution,
        new VelocityAction(_maxVelocity));
  }

  void
  _setRightAsMpiExchange() {
    typedef GhostHandlers<0, 1>                         RightHandlers;
    typedef typename RightHandlers::FghMpiExchange      FghHandler;
    typedef typename RightHandlers::PressureMpiExchange PressureHandler;
    typedef typename RightHandlers::VelocityMpiExchange VelocityHandler;

    auto leftIndent  = _grid->innerGrid.leftIndent();
    auto rightIndent = _grid->innerGrid.rightIndent();

    _handlers->mpiFghExchangeStack[0][1] =
      FghHandler::getSendHandler(_grid,
                                 _parallelDistribution,
                                 leftIndent,
                                 rightIndent);

    _handlers->mpiPressureExchangeStack[0][1] =
      PressureHandler::getReceiveHandler(_grid,
                                         _parallelDistribution,
                                         leftIndent,
                                         rightIndent);

    leftIndent(1) = 0;
    leftIndent(2) = 0;

    _handlers->mpiVelocityExchangeStack[0][1] =
      VelocityHandler::getExchangeHandler(_grid,
                                          _parallelDistribution,
                                          leftIndent,
                                          rightIndent);
  }

  void
  _setBottomAsMoving() {
    typedef GhostHandlers<1, 0> BottomHandlers;
    typedef typename BottomHandlers::MovingWallFghInitializationAction
      FghAction;
    typedef typename BottomHandlers::MovingWallFghInitialization
      FghHandler;

    typedef typename BottomHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename BottomHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename BottomHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;

    typedef typename BottomHandlers::MovingWallVelocityInitializationAction
      VelocityAction;
    typedef typename BottomHandlers::MovingWallVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[1][0] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[1][0],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[1][0] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[1][0] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));

    _handlers->velocityInitialization[1][0] =
      VelocityHandler::getHandler(
        &_grid->boundaries[1][0],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  _setBottomAsMpiExchange() {
    typedef GhostHandlers<1, 0>                          BottomHandlers;
    typedef typename BottomHandlers::FghMpiExchange      FghHandler;
    typedef typename BottomHandlers::PressureMpiExchange PressureHandler;
    typedef typename BottomHandlers::VelocityMpiExchange VelocityHandler;

    auto leftIndent  = _grid->innerGrid.leftIndent();
    auto rightIndent = _grid->innerGrid.rightIndent();

    _handlers->mpiFghExchangeStack[1][0] =
      FghHandler::getReceiveHandler(_grid,
                                    _parallelDistribution,
                                    leftIndent,
                                    rightIndent);

    _handlers->mpiPressureExchangeStack[1][0] =
      PressureHandler::getSendHandler(_grid,
                                      _parallelDistribution,
                                      leftIndent,
                                      rightIndent);

    rightIndent(0) = 0;
    rightIndent(2) = 0;

    _handlers->mpiVelocityExchangeStack[1][0] =
      VelocityHandler::getExchangeHandler(_grid,
                                          _parallelDistribution,
                                          leftIndent,
                                          rightIndent);
  }

  void
  _setTopAsMoving() {
    typedef GhostHandlers<1, 1> TopHandlers;
    typedef typename TopHandlers::MovingWallFghInitializationAction
      FghAction;
    typedef typename TopHandlers::MovingWallFghInitialization
      FghHandler;

    typedef typename TopHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename TopHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename TopHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;
    typedef typename TopHandlers::PpeRhsAcquiererAction
      PpeRhsAcquiererAction;
    typedef typename TopHandlers::PpeRhsAcquiererHandler
      PpeRhsAcquiererHandler;

    typedef typename TopHandlers::MovingWallVelocityInitializationAction
      VelocityAction;
    typedef typename TopHandlers::MovingWallVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[1][1] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[1][1],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[1][1] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[1][1] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));
    _handlers->ppeRhsAcquiererStack[1][1] =
      PpeRhsAcquiererHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsAcquiererAction());

    _handlers->velocityInitialization[1][1] =
      VelocityHandler::getHandler(
        &_grid->boundaries[1][1],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  _setTopAsMpiExchange() {
    typedef GhostHandlers<1, 1>                       TopHandlers;
    typedef typename TopHandlers::FghMpiExchange      FghHandler;
    typedef typename TopHandlers::PressureMpiExchange PressureHandler;
    typedef typename TopHandlers::VelocityMpiExchange VelocityHandler;

    auto leftIndent  = _grid->innerGrid.leftIndent();
    auto rightIndent = _grid->innerGrid.rightIndent();

    _handlers->mpiFghExchangeStack[1][1] =
      FghHandler::getSendHandler(_grid,
                                 _parallelDistribution,
                                 leftIndent,
                                 rightIndent);

    _handlers->mpiPressureExchangeStack[1][1] =
      PressureHandler::getReceiveHandler(_grid,
                                         _parallelDistribution,
                                         leftIndent,
                                         rightIndent);

    leftIndent(0) = 0;
    leftIndent(2) = 0;

    _handlers->mpiVelocityExchangeStack[1][1] =
      VelocityHandler::getExchangeHandler(_grid,
                                          _parallelDistribution,
                                          leftIndent,
                                          rightIndent);
  }

  void
  _setBackAsMoving() {
    typedef GhostHandlers<2, 0> BackHandlers;
    typedef typename BackHandlers::MovingWallFghInitializationAction
      FghAction;
    typedef typename BackHandlers::MovingWallFghInitialization
      FghHandler;

    typedef typename BackHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename BackHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename BackHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;

    typedef typename BackHandlers::MovingWallVelocityInitializationAction
      VelocityAction;
    typedef typename BackHandlers::MovingWallVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[2][0] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[2][0],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[2][0] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[2][0] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));

    _handlers->velocityInitialization[2][0] =
      VelocityHandler::getHandler(
        &_grid->boundaries[2][0],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  _setBackAsMpiExchange() {
    typedef GhostHandlers<2, 0>                        BackHandlers;
    typedef typename BackHandlers::FghMpiExchange      FghHandler;
    typedef typename BackHandlers::PressureMpiExchange PressureHandler;
    typedef typename BackHandlers::VelocityMpiExchange VelocityHandler;

    auto leftIndent  = _grid->innerGrid.leftIndent();
    auto rightIndent = _grid->innerGrid.rightIndent();

    _handlers->mpiFghExchangeStack[2][0] =
      FghHandler::getReceiveHandler(_grid,
                                    _parallelDistribution,
                                    leftIndent,
                                    rightIndent);

    _handlers->mpiPressureExchangeStack[2][0] =
      PressureHandler::getSendHandler(_grid,
                                      _parallelDistribution,
                                      leftIndent,
                                      rightIndent);

    rightIndent(0) = 0;
    rightIndent(1) = 0;

    _handlers->mpiVelocityExchangeStack[2][0] =
      VelocityHandler::getExchangeHandler(_grid,
                                          _parallelDistribution,
                                          leftIndent,
                                          rightIndent);
  }

  void
  _setFrontAsMoving() {
    typedef GhostHandlers<2, 1> FrontHandlers;
    typedef typename FrontHandlers::MovingWallFghInitializationAction
      FghAction;
    typedef typename FrontHandlers::MovingWallFghInitialization
      FghHandler;

    typedef typename FrontHandlers::PpeStencilGenerationHandler
      PpeStencilGenerationHandler;
    typedef typename FrontHandlers::PpeRhsGenerationAction
      PpeRhsGenerationAction;
    typedef typename FrontHandlers::PpeRhsGenerationHandler
      PpeRhsGenerationHandler;
    typedef typename FrontHandlers::PpeRhsAcquiererAction
      PpeRhsAcquiererAction;
    typedef typename FrontHandlers::PpeRhsAcquiererHandler
      PpeRhsAcquiererHandler;

    typedef typename FrontHandlers::MovingWallVelocityInitializationAction
      VelocityAction;
    typedef typename FrontHandlers::MovingWallVelocityInitialization
      VelocityHandler;

    _handlers->fghInitialization[2][1] =
      FghHandler::getHandler(
        &_grid->indentedBoundaries[2][1],
        _parallelDistribution,
        new FghAction(_configuration));

    _handlers->ppeStencilGeneratorStack[2][1] =
      PpeStencilGenerationHandler::getNeumannMiddle(_grid,
                                                    _parallelDistribution);
    _handlers->ppeRhsGeneratorStack[2][1] =
      PpeRhsGenerationHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsGenerationAction(0.0));
    _handlers->ppeRhsAcquiererStack[2][1] =
      PpeRhsAcquiererHandler::getHandler(
        _grid, _parallelDistribution, new PpeRhsAcquiererAction());

    _handlers->velocityInitialization[2][1] =
      VelocityHandler::getHandler(
        &_grid->boundaries[2][1],
        _parallelDistribution,
        new VelocityAction(_configuration, _maxVelocity));
  }

  void
  setFrontAsMpiExchange() {
    typedef GhostHandlers<2, 1>                         FrontHandlers;
    typedef typename FrontHandlers::FghMpiExchange      FghHandler;
    typedef typename FrontHandlers::PressureMpiExchange PressureHandler;
    typedef typename FrontHandlers::VelocityMpiExchange VelocityHandler;

    auto leftIndent  = _grid->innerGrid.leftIndent();
    auto rightIndent = _grid->innerGrid.rightIndent();

    _handlers->mpiFghExchangeStack[2][1] =
      FghHandler::getSendHandler(_grid,
                                 _parallelDistribution,
                                 leftIndent,
                                 rightIndent);

    _handlers->mpiPressureExchangeStack[2][1] =
      PressureHandler::getReceiveHandler(_grid,
                                         _parallelDistribution,
                                         leftIndent,
                                         rightIndent);

    leftIndent(0) = 0;
    leftIndent(1) = 0;

    _handlers->mpiVelocityExchangeStack[2][1] =
      VelocityHandler::getExchangeHandler(_grid,
                                          _parallelDistribution,
                                          leftIndent,
                                          rightIndent);
  }

  template <int TDimension>
  static Scalar*
  fghAccessor(CellAccessorS const& accessor) {
    return &accessor.currentCell()->fgh(TDimension);
  }

  static VelocityS*
  velocityAccessor(CellAccessorS const& accessor) {
    return &accessor.currentCell()->velocity();
  }

  static PressureS*
  pressureAccessor(CellAccessorS const& accessor) {
    return &accessor.currentCell()->pressure();
  }

  FluidSimulation::Configuration* _configuration;
  Simulation*                     _simulation;
  GridS*                          _grid;
  ParallelTopologyS*              _parallelDistribution;
  GhostCellsHandlerS*             _handlers;
  VelocityS*                      _maxVelocity;
};
}
}
#endif
