#ifndef FsiSimulation_FluidSimulation_functions_hpp
#define FsiSimulation_FluidSimulation_functions_hpp

#include <Eigen/Core>

#include <Uni/Logging/macros>

#include <mpi.h>
#include <petsc.h>

#include <cmath>
#include <limits>

namespace FsiSimulation {
namespace FluidSimulation {
template <typename TScalar>
inline void
mpiAllReduceMin(TScalar& from, TScalar& to) {}

template <>
inline void
mpiAllReduceMin<float
                >(float& from, float& to) {
  MPI_Allreduce(&from, &to, 1, MPI_FLOAT, MPI_MIN, PETSC_COMM_WORLD);
}

template <>
inline void
mpiAllReduceMin<double
                >(double& from, double& to) {
  MPI_Allreduce(&from, &to, 1, MPI_DOUBLE, MPI_MIN, PETSC_COMM_WORLD);
}

template <typename TCellAccessor, typename TScalar, int TD>
void
computeMaxVelocity(TCellAccessor const&                        accessor,
                   typename TCellAccessor::CellType::Velocity& maxVelocity) {
  maxVelocity =
    accessor.currentCell()->velocity().cwiseAbs().cwiseQuotient(
      accessor.currentWidth()).cwiseMax(maxVelocity);
}

template <typename TScalar, int TD>
struct TimeStepProcessing {
  typedef Eigen::Matrix<TScalar, TD, 1> VectorDs;

  static inline TScalar
  compute(TScalar const&  re,
          TScalar const&  tau,
          VectorDs const& minCellWidth,
          VectorDs const& maxVelocity) {
    TScalar localMin, globalMin;
    TScalar factor;

    factor = minCellWidth.cwiseProduct(minCellWidth).cwiseInverse().sum();

    localMin = std::min((TScalar)(re / (2.0 * factor)),
                        maxVelocity.cwiseInverse().minCoeff());

    globalMin = std::numeric_limits<TScalar>::max();
    mpiAllReduceMin(localMin, globalMin);

    factor  = globalMin;
    factor *= tau;

    return factor;
  }
};

template <typename TScalar>
inline TScalar
d2udx2(TScalar const& currentU,
       TScalar const& leftU,
       TScalar const& rightU,
       TScalar const& currentX,
       TScalar const& rightX) {
  TScalar const factor = currentX + rightX;

  return 2.0 * (rightU / (rightX * factor) -
                currentU / (currentX * rightX) +
                leftU / (currentX * factor));
}

template <typename TScalar>
inline TScalar
d2udy2(TScalar const& currentU,
       TScalar const& bottomU,
       TScalar const& topU,
       TScalar const& currentY,
       TScalar const& bottomY,
       TScalar const& topY) {
  TScalar const factorBottom = 0.5 * (currentY + bottomY);
  TScalar const factorTop    = 0.5 * (currentY + topY);
  TScalar const factor       = factorBottom + factorTop;

  return 2.0 * (topU / (factorTop * factor) -
                currentU / (factorBottom * factorTop) +
                bottomU / (factorBottom * factor));
}

template <typename TScalar>
inline TScalar
duvdy(TScalar const& currentU,
      TScalar const& currentV,
      TScalar const& bottomU,
      TScalar const& bottomV,
      TScalar const& rightBottomV,
      TScalar const& rightV,
      TScalar const& topU,
      TScalar const& currentY,
      TScalar const& currentX,
      TScalar const& bottomY,
      TScalar const& topY,
      TScalar const& rightX,
      TScalar const& gamma) {
  TScalar const yCurrent = 0.5 * currentY;
  TScalar const yBottom  = 0.5 * (currentY + bottomY);
  TScalar const yTop     = 0.5 * (currentY + topY);
  TScalar const xCurrent = 0.5 * currentX;
  TScalar const xRight   = 0.5 * (currentX + rightX);

  TScalar const secondOrder =
    (((xRight - xCurrent) / xRight * currentV + xCurrent / xRight * rightV) *
     ((yTop - yCurrent) / yTop * currentU + yCurrent / yTop * topU) -
     ((xRight - xCurrent) / xRight * bottomV + xCurrent / xRight *
      rightBottomV) *
     ((yBottom - yCurrent) / yBottom * currentU + yCurrent / yBottom * bottomU)
    ) / (2.0 * yCurrent);

  TScalar const kr = (xRight - xCurrent) / xRight * currentV +
                     xCurrent / xRight * rightV;
  TScalar const kl = (xRight - xCurrent) / xRight * bottomV +
                     xCurrent / xRight * rightBottomV;

  TScalar const firstOrder = 1.0 / (4.0 * yCurrent) * (
    kr * (currentU + topU) - kl * (bottomU + currentU) +
    std::fabs(kr) * (currentU - topU) -
    std::fabs(kl) * (bottomU - currentU));

  TScalar const result = (1.0 - gamma) * secondOrder + gamma * firstOrder;

  return result;
}

template <typename TScalar>
inline TScalar
du2dx(TScalar const& currentU,
      TScalar const& leftU,
      TScalar const& rightU,
      TScalar const& currentX,
      TScalar const& leftX,
      TScalar const& rightX,
      TScalar const& gamma) {
  TScalar const dxShort = 0.5 * currentX;
  TScalar const dxLong0 = 0.5 * (leftX + currentX);
  TScalar const dxLong1 = 0.5 * (currentX + rightX);

  TScalar const kr =
    (dxLong1 - dxShort) / dxLong1 * currentU + dxShort / dxLong1 * rightU;

  TScalar const kl =
    (dxLong0 - dxShort) / dxLong0 * currentU + dxShort / dxLong0 * leftU;

  TScalar const secondOrder = (kr * kr - kl * kl) / (2.0 * dxShort);

  TScalar const firstOrder = 1.0 / (4.0 * dxShort) *
                             (kr * (currentU + rightU) -
                              kl * (leftU + currentU) +
                              std::fabs(kr) * (currentU - rightU) -
                              std::fabs(kl) * (leftU - currentU));

  TScalar const result = (1.0 - gamma) * secondOrder + gamma * firstOrder;

  return result;
}

template <typename TScalar>
inline TScalar
computeFGH2D(TScalar const& currentU,
             TScalar const& currentV,
             TScalar const& leftU,
             TScalar const& rightU,
             TScalar const& rightV,
             TScalar const& bottomU,
             TScalar const& bottomV,
             TScalar const& topU,
             TScalar const& rightBottomV,
             TScalar const& currentX,
             TScalar const& currentY,
             TScalar const& leftX,
             TScalar const& rightX,
             TScalar const& bottomY,
             TScalar const& topY,
             TScalar const& re,
             TScalar const& gamma,
             TScalar const& gx,
             TScalar const& dt) {
  return currentU
         + dt * (1.0 / re *
                 (d2udx2(currentU,
                         leftU,
                         rightU,
                         currentX,
                         rightX)
                  + d2udy2(currentU,
                           bottomU,
                           topU,
                           currentY,
                           bottomY,
                           topY))
                 - du2dx(currentU,
                         leftU,
                         rightU,
                         currentX,
                         leftX,
                         rightX,
                         gamma)
                 - duvdy(currentU,
                         currentV,
                         bottomU,
                         bottomV,
                         rightBottomV,
                         rightV,
                         topU,
                         currentY,
                         currentX,
                         bottomY,
                         topY,
                         rightX,
                         gamma)
                 + gx);
}

template <typename TScalar>
inline TScalar
computeFGH3D(TScalar const& currentU,
             TScalar const& currentV,
             TScalar const& currentW,
             TScalar const& leftU,
             TScalar const& rightU,
             TScalar const& rightV,
             TScalar const& rightW,
             TScalar const& bottomU,
             TScalar const& bottomV,
             TScalar const& topU,
             TScalar const& backU,
             TScalar const& backW,
             TScalar const& frontU,
             TScalar const& rightBottomV,
             TScalar const& rightBackW,
             TScalar const& currentX,
             TScalar const& currentY,
             TScalar const& currentZ,
             TScalar const& leftX,
             TScalar const& rightX,
             TScalar const& bottomY,
             TScalar const& topY,
             TScalar const& backZ,
             TScalar const& frontZ,
             TScalar const& re,
             TScalar const& gamma,
             TScalar const& gx,
             TScalar const& dt) {
  return currentU
         + dt * (1.0 / re *
                 (d2udx2(currentU,
                         leftU,
                         rightU,
                         currentX,
                         rightX) +
                  d2udy2(currentU,
                         bottomU,
                         topU,
                         currentY,
                         bottomY,
                         topY) +
                  d2udy2(currentU, // d2udz2
                         backU,
                         frontU,
                         currentZ,
                         backZ,
                         frontZ)) -
                 du2dx(currentU,
                       leftU,
                       rightU,
                       currentX,
                       leftX,
                       rightX,
                       gamma) -
                 duvdy(currentU,
                       currentV,
                       bottomU,
                       bottomV,
                       rightBottomV,
                       rightV,
                       topU,
                       currentY,
                       currentX,
                       bottomY,
                       topY,
                       rightX,
                       gamma) -
                 duvdy(currentU, // duwdz
                       currentW,
                       backU,
                       backW,
                       rightBackW,
                       rightW,
                       frontU,
                       currentZ,
                       currentX,
                       backZ,
                       frontZ,
                       rightX,
                       gamma) +
                 gx);
}

template <typename TCellAccessor,
          typename TSimulationParameters,
          typename TScalar,
          int TD>
class FghProcessing {
  static inline void
  compute(TCellAccessor const&         accessor,
          TSimulationParameters const& simulationParameters,
          TScalar const&               dt) {}
};

template <typename TCellAccessor,
          typename TSimulationParameters,
          typename TScalar>
class FghProcessing<TCellAccessor, TSimulationParameters, TScalar, 3> {
public:
  static inline void
  compute(TCellAccessor const&         accessor,
          TSimulationParameters const& simulationParameters,
          TScalar const&               dt) {
    for (int d1 = 0; d1 < 3; ++d1) {
      int d2 = d1 + 1;
      int d3 = d2 + 1;

      if (d1 == 1) {
        d2 = 0;
        d3 = 2;
      } else if (d1 == 2) {
        d2 = 0;
        d3 = 1;
      }
      accessor.currentCell()->fgh(d1) = computeFGH3D(
        accessor.currentCell()->velocity(d1),
        accessor.currentCell()->velocity(d2),
        accessor.currentCell()->velocity(d3),
        accessor.leftCellInDimension(d1)->velocity(d1),
        accessor.rightCellInDimension(d1)->velocity(d1),
        accessor.rightCellInDimension(d1)->velocity(d2),
        accessor.rightCellInDimension(d1)->velocity(d3),
        accessor.leftCellInDimension(d2)->velocity(d1),
        accessor.leftCellInDimension(d2)->velocity(d2),
        accessor.rightCellInDimension(d2)->velocity(d1),
        accessor.leftCellInDimension(d3)->velocity(d1),
        accessor.leftCellInDimension(d3)->velocity(d3),
        accessor.rightCellInDimension(d3)->velocity(d1),
        accessor.leftRightCellInDimensions(d2, d1)->velocity(d2),
        accessor.leftRightCellInDimensions(d3, d1)->velocity(d3),
        accessor.currentWidth() (d1),
        accessor.currentWidth() (d2),
        accessor.currentWidth() (d3),
        accessor.leftWidthInDimension (d1)(d1),
        accessor.rightWidthInDimension (d1)(d1),
        accessor.leftWidthInDimension (d2)(d2),
        accessor.rightWidthInDimension (d2)(d2),
        accessor.leftWidthInDimension (d3)(d3),
        accessor.rightWidthInDimension (d3)(d3),
        simulationParameters.re(),
        simulationParameters.gamma(),
        simulationParameters.g(d1),
        dt);
    }
  }
};

template <typename TCellAccessor,
          typename TSimulationParameters,
          typename TScalar>
class FghProcessing<TCellAccessor, TSimulationParameters, TScalar, 2> {
public:
  static inline void
  compute(TCellAccessor const&         accessor,
          TSimulationParameters const& simulationParameters,
          TScalar const&               dt) {
    for (int d1 = 0; d1 < 2; ++d1) {
      int d2 = d1 + 1;

      if (d1 == 1) {
        d2 = 0;
      }
      accessor.currentCell()->fgh(d1) = computeFGH2D(
        accessor.currentCell()->velocity(d1),
        accessor.currentCell()->velocity(d2),
        accessor.leftCellInDimension(d1)->velocity(d1),
        accessor.rightCellInDimension(d1)->velocity(d1),
        accessor.rightCellInDimension(d1)->velocity(d2),
        accessor.leftCellInDimension(d2)->velocity(d1),
        accessor.leftCellInDimension(d2)->velocity(d2),
        accessor.rightCellInDimension(d2)->velocity(d1),
        accessor.leftRightCellInDimensions(d2, d1)->velocity(d2),
        accessor.currentWidth() (d1),
        accessor.currentWidth() (d2),
        accessor.leftWidthInDimension (d1)(d1),
        accessor.rightWidthInDimension (d1)(d1),
        accessor.leftWidthInDimension (d2)(d2),
        accessor.rightWidthInDimension (d2)(d2),
        simulationParameters.re(),
        simulationParameters.gamma(),
        simulationParameters.g(d1),
        dt);
    }
  }
};

template <typename TCellAccessor, typename TScalar, int TD>
class RhsProcessing {
public:
  static inline TScalar
  compute(TCellAccessor const& accessor,
          TScalar const&       dt) {
    TScalar result = 0.0;

    for (int d = 0; d < TD; ++d) {
      result += (accessor.currentCell()->fgh(d)
                 - accessor.leftCellInDimension(d)->fgh(d))
                / accessor.currentWidth() (d);
    }

    result = 1.0 / dt * result;

    return result;
  }
};

template <typename TParallelTopology,
          typename TCellAccessor,
          typename TScalar,
          int TD>
class PressurePoissonStencilProcessing {
public:
  template <typename TStencil,
            typename TRow,
            typename TColumns>
  static inline void
  compute(TParallelTopology const* topology,
          TCellAccessor const&     accessor,
          TStencil&                stencil,
          TRow&                    row,
          TColumns&                columns) {
    typedef Eigen::Matrix<TScalar, 2* TD, 1> Vector2Ds;

    auto corner = topology->corner;

    Vector2Ds meanWidths;

    for (int d = 0; d < TD; ++d) {
      meanWidths(2 * d) = 0.5 *
                          (accessor.currentWidth() (d) +
                           accessor.leftWidthInDimension (d)(d));
      meanWidths(2 * d + 1) = 0.5 *
                              (accessor.currentWidth() (d) +
                               accessor.rightWidthInDimension (d)(d));

      auto leftIndex = accessor.leftIndexInDimension(d);
      leftIndex += corner;
      auto rightIndex = accessor.rightIndexInDimension(d);
      rightIndex += corner;

      columns[2 * d].i     = leftIndex(0);
      columns[2 * d].j     = leftIndex(1);
      columns[2 * d + 1].i = rightIndex(0);
      columns[2 * d + 1].j = rightIndex(1);

      if (TD == 3) {
        columns[2 * d].k     = leftIndex(2);
        columns[2 * d + 1].k = rightIndex(2);
      }
    }
    auto currentIndex = accessor.currentIndex();
    currentIndex     += corner;
    columns[2 * TD].i = currentIndex(0);
    columns[2 * TD].j = currentIndex(1);

    if (TD == 3) {
      columns[2 * TD].k = currentIndex(2);
    }
    row = columns[2 * TD];

    stencil[2 * TD] = 0;

    for (int d = 0; d < TD; ++d) {
      auto meanLeftAndRightWidth = meanWidths(2 * d) + meanWidths(2 * d + 1);
      stencil[2 * d] =
        2.0 / (meanWidths(2 * d) * meanLeftAndRightWidth);
      stencil[2 * d + 1] =
        2.0 / (meanWidths(2 * d + 1) * meanLeftAndRightWidth);
      stencil[2 * TD] -= 2.0 / (meanWidths(2 * d) * meanWidths(2 * d + 1));
    }
  }
};

template <typename TCellAccessor, typename TScalar, int TD>
class VelocityProcessing {
public:
  static inline void
  compute(TCellAccessor const& accessor,
          int const&           dimension,
          TScalar const&       dt) {
    accessor.currentCell()->velocity() (dimension) =
      accessor.currentCell()->fgh() (dimension) -
      dt / (0.5 * (accessor.rightWidthInDimension (dimension)(dimension) +
                   accessor.currentWidth() (dimension)))
      * (accessor.rightCellInDimension(dimension)->pressure() -
         accessor.currentCell()->pressure());
  }
};
}
}

#endif