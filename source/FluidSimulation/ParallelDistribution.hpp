#pragma once

#include <Uni/ExecutionControl/exception>
#include <Uni/Logging/format>
#include <Uni/Logging/macros>

#include <Eigen/Core>

#include <petscsys.h>

#include <array>
#include <cstdlib>

namespace FsiSimulation {
namespace FluidSimulation {
template <int TDimensions>
class ParallelDistribution;

template <int TDimensions>
void
logParallelTopologyInfo(ParallelDistribution<TDimensions> const& topology);

template <int TDimensions>
class ParallelDistribution {
public:
  enum {
    Dimensions = TDimensions
  };

  using VectorDi  =  Eigen::Matrix<int, TDimensions, 1>;

  using Neighbors = std::array<std::array<int, 2>, TDimensions>;

public:
  ParallelDistribution() : mpiCommunicator(PETSC_COMM_WORLD) {}

  ParallelDistribution(ParallelDistribution const& other) = delete;

  ~ParallelDistribution() {}

  ParallelDistribution&
  operator=(ParallelDistribution const& other) = delete;

  void
  initialize(VectorDi const& processorSize_,
             VectorDi const& globalSize_) {
    int processCount;
    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
    MPI_Comm_size(PETSC_COMM_WORLD, &processCount);

    if (processCount != processorSize_.prod()) {
      throwException("Wrong number of nodes: executed with {1} processors, "
                     "configured with {2} ({3}) processors",
                     processCount,
                     processorSize_.prod(),
                     processorSize_.transpose());
    }

    processorSize        = processorSize_;
    uniformLocalCellSize = globalSize_.cwiseQuotient(processorSize);
    localCellSize        = uniformLocalCellSize;
    lastLocalCellSize    = uniformLocalCellSize;
    globalCellSize       = uniformLocalCellSize.cwiseProduct(processorSize);

    auto tempDivRank = rank;
    int  tempDivSize = 1;

    for (int i = 0; i < (TDimensions - 1); ++i) {
      tempDivSize = processorSize(i);
      auto tempDiv = std::div(tempDivRank, tempDivSize);
      index(i)    = tempDiv.rem;
      corner(i)   = index(i) * uniformLocalCellSize(i);
      tempDivRank = tempDiv.quot;
    }
    index(TDimensions - 1)  = tempDivRank;
    corner(TDimensions - 1) = index(TDimensions - 1) * uniformLocalCellSize(TDimensions - 1);

    for (int d = 0; d < TDimensions; ++d) {
      auto tempIndex = index;
      tempIndex(d)   -= 1;
      neighbors[d][0] = getRank(tempIndex);
      tempIndex       = index;
      tempIndex(d)   += 1;
      neighbors[d][1] = getRank(tempIndex);
    }

    for (int d = 0; d < TDimensions; ++d) {
      if (globalCellSize(d) != globalSize_(d)) {
        auto diff = globalSize_(d) - globalCellSize(d);

        if (neighbors[d][1] < 0) {
          localCellSize(d) += diff;
        }

        lastLocalCellSize(d) += diff;
        globalCellSize(d)     = globalSize_(d);
      }
    }
  }

  int
  getRank(VectorDi const& index_) {
    auto result   = 0;
    int  tempSize = 1;

    for (int i = 0; i < TDimensions; ++i) {
      if (index_(i) < 0 || index_(i) >= processorSize(i)) {
        return -1;
      }
      result   += index_(i) * tempSize;
      tempSize *= processorSize(i);
    }

    return result;
  }

  VectorDi processorSize;
  VectorDi globalCellSize;

  int       rank;
  VectorDi  localCellSize;
  VectorDi  uniformLocalCellSize;
  VectorDi  lastLocalCellSize;
  VectorDi  index;
  VectorDi  corner;
  Neighbors neighbors;
  MPI_Comm  mpiCommunicator;
};

template <int TDimensions>
void
logParallelTopologyInfo(ParallelDistribution<TDimensions> const& topology) {
  std::string neighbors;

  for (int d = 0; d < TDimensions; ++d) {
    neighbors += Uni::Logging::format("{1} ", topology.neighbors[d][0]);
    neighbors += Uni::Logging::format("{1} ", topology.neighbors[d][1]);
  }
  logInfo("ParallelDistribution current rank:     {1}\n"
          "ParallelDistribution processor size:   {2}\n"
          "ParallelDistribution global cell size: {3}\n"
          "ParallelDistribution local cell size:  {4}\n"
          "ParallelDistribution current index:    {5}\n"
          "ParallelDistribution corner:           {6}\n"
          "ParallelDistribution neighbors:        {7}\n",
          topology.rank,
          topology.processorSize.transpose(),
          topology.globalCellSize.transpose(),
          topology.localCellSize.transpose(),
          topology.index.transpose(),
          topology.corner.transpose(),
          neighbors);
}
}
}
