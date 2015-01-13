#ifndef FsiSimulation_FluidSimulation_Private_petscgenerics_hpp
#define FsiSimulation_FluidSimulation_Private_petscgenerics_hpp

#include <Eigen/Core>

#include <memory>
#include <petscdm.h>
#include <petscdmda.h>
#include <petscksp.h>

namespace FsiSimulation {
namespace FluidSimulation {
template <int TD>
using DMBoundaryTypeVector = Eigen::Matrix<DMBoundaryType, TD, 1>;
template <int TD>
using VectorDi = Eigen::Matrix<int, TD, 1>;

typedef std::unique_ptr<PetscInt const[]> UniqueConstPetscIntArray;
template <int TD>
using VectorDConstPetscIntPointer =
        Eigen::Matrix<UniqueConstPetscIntArray, TD, 1>;

template <int TD>
DMBoundaryTypeVector<TD>
createDMBoundaries() {
  DMBoundaryTypeVector<TD> result;

  for (int i = 0; i < TD; ++i) {
    result(i) = DM_BOUNDARY_NONE;
  }

  return result;
}

inline void
setupCustomOptions(KSP& context,
                   PC&  preconditioner) {
  KSPSetType(context, KSPFGMRES);

  int comm_size;
  MPI_Comm_size(PETSC_COMM_WORLD, &comm_size);

  if (comm_size == 1) {
    // if serial
    PCSetType(preconditioner, PCILU);
    PCFactorSetLevels(preconditioner, 1);
    KSPSetPC(context, preconditioner);
  } else {
    // if parallel
    PCSetType(preconditioner, PCASM);
    KSPSetPC(context, preconditioner);
  }

  KSPSetFromOptions(context);
  KSPSetInitialGuessNonzero(context, PETSC_TRUE);
  KSPSetUp(context);

  // from here we can change sub_ksp if necessary
  // that has to be done after setup. The other solvers above
  // can be changed before setup with KSPSetFromOptions

  if (comm_size > 1) {
    KSP* subksp;
    PC   subpc;

    PCASMGetSubKSP(preconditioner, NULL, NULL, &subksp);
    KSPGetPC(subksp[0], &subpc);

    PetscBool has_fl;
    PetscBool has_sub_type;
    PetscOptionsHasName(NULL, "-sub_pc_factor_levels", &has_fl);
    PetscOptionsHasName(NULL, "-sub_pc_type",          &has_sub_type);

    if (!(has_sub_type)) {
      PCSetType(subpc, PCILU);
    }

    if (!(has_fl)) {
      PCFactorSetLevels(subpc, 1);
    }

    KSPSetUp(context);
  }
}

template <int TD>
void
DMCreate(MPI_Comm const&                        comm,
         DMBoundaryTypeVector<TD> const&        boundaryTypes,
         DMDAStencilType const&                 stencilType,
         VectorDi<TD> const&                    globalSize,
         VectorDi<TD> const&                    processorSize,
         int const&                             dof,
         int const&                             stencilWidth,
         VectorDConstPetscIntPointer<TD> const& localSizes,
         DM*                                    da) {}

template <>
void
DMCreate<2
         >(MPI_Comm const&                       comm,
           DMBoundaryTypeVector<2> const&        boundaryTypes,
           DMDAStencilType const&                stencilType,
           VectorDi<2> const&                    globalSize,
           VectorDi<2> const&                    processorSize,
           int const&                            dof,
           int const&                            stencilWidth,
           VectorDConstPetscIntPointer<2> const& localSizes,
           DM*                                   da) {
  DMDACreate2d(comm,
               boundaryTypes(0),
               boundaryTypes(1),
               stencilType,
               globalSize(0),
               globalSize(1),
               processorSize(0),
               processorSize(1),
               dof,
               stencilWidth,
               localSizes(0).get(),
               localSizes(1).get(),
               da);
}

template <>
void
DMCreate<3
         >(MPI_Comm const&                       comm,
           DMBoundaryTypeVector<3> const&        boundaryTypes,
           DMDAStencilType const&                stencilType,
           VectorDi<3> const&                    globalSize,
           VectorDi<3> const&                    processorSize,
           int const&                            dof,
           int const&                            stencilWidth,
           VectorDConstPetscIntPointer<3> const& localSizes,
           DM*                                   da) {
  DMDACreate3d(comm,
               boundaryTypes(0),
               boundaryTypes(1),
               boundaryTypes(2),
               stencilType,
               globalSize(0),
               globalSize(1),
               globalSize(2),
               processorSize(0),
               processorSize(1),
               processorSize(2),
               dof,
               stencilWidth,
               localSizes(0).get(),
               localSizes(1).get(),
               localSizes(2).get(),
               da);
}

template <int TD>
class petscgenerics {
public:
  petscgenerics() {}

  petscgenerics(petscgenerics const& other) {}

  ~petscgenerics() {}

  petscgenerics&
  operator=(petscgenerics const& other) {
    return *this;
  }
};

template <>
class petscgenerics<2> {
public:
  petscgenerics() {}

  petscgenerics(petscgenerics const& other) {}

  ~petscgenerics() {}

  petscgenerics&
  operator=(petscgenerics const& other) {
    return *this;
  }
};

template <>
class petscgenerics<3> {
public:
  petscgenerics() {}

  petscgenerics(petscgenerics const& other) {}

  ~petscgenerics() {}

  petscgenerics&
  operator=(petscgenerics const& other) {
    return *this;
  }
};
}
}
#endif