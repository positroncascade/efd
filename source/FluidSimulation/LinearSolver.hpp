#ifndef FsiSimulation_FluidSimulation_Solvers_LinearSolver_hpp
#define FsiSimulation_FluidSimulation_Solvers_LinearSolver_hpp

#include "ParallelDistribution.hpp"
#include "Private/petscgenerics.hpp"
#include "StructuredMemory/Pointers.hpp"
#include "functions.hpp"

#include "GhostLayer/PetscExchangeHandler.hpp"
#include "GhostLayer/PressureStencilHanler.hpp"

#include <petscdm.h>
#include <petscdmda.h>
#include <petscksp.h>

#include <Uni/Logging/macros>

#include <functional>

namespace FsiSimulation {
namespace FluidSimulation {
template <typename TGrid,
          typename TStencilGenerator,
          typename TRhsGenerator,
          typename TResultAcquirer>
class LinearSolver {
public:
  typedef TGrid GridType;

  typedef typename GridType::CellAccessorType CellAccessorType;

  typedef typename CellAccessorType::ScalarType ScalarType;

  enum {
    Dimensions = CellAccessorType::Dimensions
  };

  typedef ParallelDistribution<Dimensions> ParallelDistributionType;

  typedef
    GhostLayer::LsStencilGenerator::FunctorStack<Dimensions>
    GhostStencilGenerator;

  typedef
    GhostLayer::PetscExchange::FunctorStack<Dimensions>
    GhostRhsGenerator;

  typedef
    GhostLayer::PetscExchange::FunctorStack<Dimensions>
    GhostRhsAcquierer;

  typedef Eigen::Matrix<ScalarType, Dimensions, 1> VectorDsType;

  typedef Eigen::Matrix<int, Dimensions, 1> VectorDiType;

public:
  LinearSolver() {}

  void
  initialize(GridType const*                 grid,
             ParallelDistributionType const* parallelDistribution,
             TStencilGenerator const*        stencilGenerator,
             TRhsGenerator const*            rhsGenerator,
             TResultAcquirer const*          resultAcquierer,
             GhostStencilGenerator const*    ghostStencilGenerator,
             GhostRhsGenerator const*        ghostRhsGenerator,
             GhostRhsAcquierer const*        ghostRhsAcquierer) {
    _grid                  = grid;
    _parallelDistribution  = parallelDistribution;
    _stencilGenerator      = stencilGenerator;
    _rhsGenerator          = rhsGenerator;
    _resultAcquierer       = resultAcquierer;
    _ghostStencilGenerator = ghostStencilGenerator;
    _ghostRhsGenerator     = ghostRhsGenerator;
    _ghostRhsAcquierer     = ghostRhsAcquierer;
    KSPCreate(PETSC_COMM_WORLD, &_context);
    PCCreate(PETSC_COMM_WORLD, &_preconditioner);

    VectorDConstPetscIntPointer<Dimensions> localSizes;

    for (int d = 0; d < Dimensions; ++d) {
      auto array = new PetscInt[_parallelDistribution->processorSize(d)];
      localSizes(d) = UniqueConstPetscIntArray(array);

      for (int j = 0; j < _parallelDistribution->processorSize(d) - 1; ++j) {
        array[j] = _parallelDistribution->uniformLocalCellSize(d);
      }
      array[_parallelDistribution->processorSize(d) - 1]
        = _parallelDistribution->lastLocalCellSize(d) + 1;
      ++array[0];
    }

    DMCreate<Dimensions>(PETSC_COMM_WORLD,
                         createDMBoundaries<Dimensions>(),
                         DMDA_STENCIL_STAR,
                         _parallelDistribution->globalCellSize
                         + 2 * VectorDiType::Ones(),
                         _parallelDistribution->processorSize,
                         1,
                         2,
                         localSizes,
                         &_da);

    DMCreateGlobalVector(_da, &_x);
    KSPSetDM(_context, _da);

    KSPSetComputeOperators(_context, computeMatrix, this);

    setupCustomOptions(_context, _preconditioner);
    // DMDALocalInfo petscInfo;
    // DMDAGetLocalInfo(_da, &petscInfo);
    // logInfo("dim {1} dof {2} sw {3} \n"
    // "global number of grid points in each direction {4} {5} {6} \n"
    // "starting point of this processor, excluding ghosts {7} {8} {9} \n"
    // "number of grid points on this processor, excluding ghosts {10} {11} {12}
    // \n"
    // "starting point of this processor including ghosts {13} {14} {15} \n"
    // "number of grid points on this processor including ghosts {16} {17} {18}
    // \n",
    // petscInfo.dim, petscInfo.dof, petscInfo.sw,
    // petscInfo.mx, petscInfo.my, petscInfo.mz,
    // petscInfo.xs, petscInfo.ys, petscInfo.zs,
    // petscInfo.xm, petscInfo.ym, petscInfo.zm,
    // petscInfo.gxs, petscInfo.gys, petscInfo.gzs,
    // petscInfo.gxm, petscInfo.gym, petscInfo.gzm);

    // int _firstX;
    // int _firstY;
    // int _firstZ;
    // int _lengthX;
    // int _lengthY;
    // int _lengthZ;
    // DMDAGetCorners(_da,
    // &_firstX,
    // &_firstY,
    // &_firstZ,
    // &_lengthX,
    // &_lengthY,
    // &_lengthZ);

    // logInfo("Corner beginning {1} {2} {3}\n"
    // "Corener end      {4} {5} {6}\n",
    // _firstX, _firstY, _firstZ,
    // _firstX + _lengthX, _firstY + _lengthY, _firstZ + _lengthZ);
  }

  void
  update() {
    KSPSetComputeOperators(_context, computeMatrix, this);
    setupCustomOptions(_context, _preconditioner);
  }

  void
  solve() {
    typedef StructuredMemory::Pointers<PetscScalar, Dimensions> Pointers;

    KSPSetComputeRHS(_context, computeRHS, this);
    KSPSolve(_context, PETSC_NULL, _x);

    typename Pointers::Type array;
    DMDAVecGetArray(_da, _x, &array);

    auto corner = _parallelDistribution->corner;

    for (auto const& accessor : _grid->innerGrid) {
      auto index = accessor.indexValues();
      index += corner;

      _resultAcquierer->set(accessor,
                            Pointers::dereference(array, index));
    }

    for (int d = 0; d < Dimensions; ++d) {
      for (int d2 = 0; d2 < 2; ++d2) {
        (*_ghostRhsAcquierer)[d][d2](array);
      }
    }

    DMDAVecRestoreArray(_da, _x, &array);
  }

private:
  static PetscErrorCode
#if ((PETSC_VERSION_MAJOR == 3) && (PETSC_VERSION_MINOR >= 5))
  computeMatrix(KSP ksp, Mat A, Mat pc, void* ctx) {
#else
  computeMatrix(KSP ksp, Mat A, Mat pc, MatStructure * matStructure,
                void* ctx) {
#endif
    auto solver = static_cast<LinearSolver*>(ctx);

    PetscScalar stencil[2 * Dimensions + 1];
    MatStencil  row;
    MatStencil  columns[2 * Dimensions + 1];

    for (auto const& accessor : solver->_grid->innerGrid) {
      solver->_stencilGenerator->get(accessor,
                                     stencil,
                                     row,
                                     columns);

      MatSetValuesStencil(A, 1, &row, 2 * Dimensions + 1, columns, stencil,
                          INSERT_VALUES);
    }

    for (int d = 0; d < Dimensions; ++d) {
      for (int d2 = 0; d2 < 2; ++d2) {
        (*solver->_ghostStencilGenerator)[d][d2](A);
      }
    }

    MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);

    MatNullSpace nullspace;
    MatNullSpaceCreate(PETSC_COMM_WORLD, PETSC_TRUE, 0, 0, &nullspace);
    MatSetNullSpace(A, nullspace);
    MatNullSpaceDestroy(&nullspace);

    return 0;
  }

  static PetscErrorCode
  computeRHS(KSP ksp, Vec b, void* ctx) {
    auto solver = static_cast<LinearSolver*>(ctx);
    typedef StructuredMemory::Pointers<PetscScalar, Dimensions> Pointers;
    typename Pointers::Type array;

    DM da;
    KSPGetDM(ksp, &da);
    DMDAVecGetArray(da, b, &array);

    auto corner = solver->_parallelDistribution->corner;

    for (auto const& accessor : solver->_grid->innerGrid) {
      auto value = solver->_rhsGenerator->get(accessor);

      auto index = accessor.indexValues();
      index                              += corner;
      Pointers::dereference(array, index) = value;
    }

    for (int d = 0; d < Dimensions; ++d) {
      for (int d2 = 0; d2 < 2; ++d2) {
        (*solver->_ghostRhsGenerator)[d][d2](array);
      }
    }

    DMDAVecRestoreArray(da, b, &array);
    VecAssemblyBegin(b);
    VecAssemblyEnd(b);

    return 0;
  }

  GridType const*                 _grid;
  ParallelDistributionType const* _parallelDistribution;
  TStencilGenerator const*        _stencilGenerator;
  TRhsGenerator const*            _rhsGenerator;
  TResultAcquirer const*          _resultAcquierer;
  GhostStencilGenerator const*    _ghostStencilGenerator;
  GhostRhsGenerator const*        _ghostRhsGenerator;
  GhostRhsAcquierer const*        _ghostRhsAcquierer;

  Vec _x;
  DM  _da;
  KSP _context;
  PC  _preconditioner;
};
}
}

#endif
