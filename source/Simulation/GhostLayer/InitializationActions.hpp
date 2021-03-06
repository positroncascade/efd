#pragma once

#include "Private/utilities.hpp"

#include "Simulation/Configuration.hpp"
#include "Simulation/computemaxvelocity.hpp"

namespace Fluid {
namespace Simulation {
namespace GhostLayer {
namespace Initialization {
template <typename TSolverTraits,
          int TDimension,
          int TDirection>
class InputVelocityAction {
public:
  using SolverTraitsType = TSolverTraits;

  enum {
    Dimensions = SolverTraitsType::Dimensions
  };

  using GridType = typename SolverTraitsType::GridType;

  using CellAccessorType = typename SolverTraitsType::CellAccessorType;

  using VectorDsType = typename SolverTraitsType::VectorDsType;

  using VectorDiType = typename SolverTraitsType::VectorDiType;

  using ScalarType = typename SolverTraitsType::ScalarType;

  InputVelocityAction(Configuration* parameters,
                      VectorDsType*  maxVelocity)
    : _configuration(parameters),
    _maxVelocity(maxVelocity) {}

  void
  setValue(CellAccessorType const& accessor) {
    int offset = -1;

    if (TDirection == 0) {
      offset = +1;
    }

    for (int d = 0; d < Dimensions; ++d) {
      if (d == TDimension) {
        if (TDirection == 0) {
          accessor.velocity(TDimension)
            = _configuration->walls[TDimension][TDirection]->velocity()
                (TDimension);
          compute_max_velocity(accessor.velocity(TDimension),
                               accessor.width(TDimension),
                               d,
                               *_maxVelocity);
        } else {
          accessor.velocity(TDimension, -1, TDimension)
            = _configuration->walls[TDimension][TDirection]->velocity()
                (TDimension);
          compute_max_velocity(accessor.velocity(TDimension, -1, TDimension),
                               accessor.width(TDimension, -1, TDimension),
                               d,
                               *_maxVelocity);
        }
      } else {
        accessor.velocity(d)
          = 2.0 * _configuration->walls[TDimension][TDirection]
            ->velocity() (d) - accessor.velocity(TDimension, offset, d);
        compute_max_velocity(accessor.velocity(d),
                             accessor.width(d),
                             d,
                             *_maxVelocity);
      }
    }

    // logInfo("{1} {2} {3}", TDimension, TDirection,
    //         accessor.velocity().transpose());

    // if (accessor.velocity().cwiseAbs().maxCoeff() > 1.e+10) {
    //   logInfo("Got you {1} {2}",
    //           accessor.index().transpose(),
    //           accessor.velocity().transpose(),
    //           accessor.velocity(TDimension, offset).transpose());
    // }
  }

  Configuration* _configuration;
  VectorDsType   _;
  VectorDsType*  _maxVelocity;
};

template <typename TSolverTraits,
          int TDimension,
          int TDirection>
class ParabolicInputVelocityAction {
public:
  using SolverTraitsType = TSolverTraits;

  enum {
    Dimensions = SolverTraitsType::Dimensions
  };

  using GridType = typename SolverTraitsType::GridType;

  using CellAccessorType = typename SolverTraitsType::CellAccessorType;

  using VectorDsType = typename SolverTraitsType::VectorDsType;

  using VectorDiType = typename SolverTraitsType::VectorDiType;

  using ScalarType = typename SolverTraitsType::ScalarType;

  ParabolicInputVelocityAction(Configuration* parameters,
                               VectorDsType*  maxVelocity)
    : _configuration(parameters),
    _maxVelocity(maxVelocity) {}

  void
  setValue(CellAccessorType const& accessor) {
    int offset = -1;

    if (TDirection == 0) {
      offset = +1;
    }

    ScalarType result;

    for (int d = 0; d < Dimensions; ++d) {
      if (d == TDimension) {
        result
          = _configuration->walls[TDimension][TDirection]->velocity()
              (TDimension);

        if (TDirection == 0) {
          compute_parabolic_input_velocity(accessor, TDimension, result);
          accessor.velocity(TDimension) = result;
          compute_max_velocity(accessor.velocity(TDimension),
                               accessor.width(TDimension),
                               d,
                               *_maxVelocity);
        } else {
          compute_parabolic_input_velocity(
            accessor.relative(TDimension, -1),
            TDimension,
            result);
          accessor.velocity(TDimension, -1, TDimension) = result;
          compute_max_velocity(accessor.velocity(TDimension, -1, TDimension),
                               accessor.width(TDimension, -1, TDimension),
                               d,
                               *_maxVelocity);
        }
      } else {
        accessor.velocity(d)
          = 2.0 * _configuration->walls[TDimension][TDirection]
            ->velocity() (d) - accessor.velocity(TDimension, offset, d);
        compute_max_velocity(accessor.velocity(d),
                             accessor.width(d),
                             d,
                             *_maxVelocity);
      }
    }
  }

  Configuration* _configuration;
  VectorDsType*  _maxVelocity;
};

template <typename TSolverTraits,
          int TDimension,
          int TDirection>
class OutputVelocityAction {
public:
  using SolverTraitsType = TSolverTraits;

  enum {
    Dimensions = SolverTraitsType::Dimensions
  };

  using GridType = typename SolverTraitsType::GridType;

  using CellAccessorType = typename SolverTraitsType::CellAccessorType;

  using VectorDsType = typename SolverTraitsType::VectorDsType;

  using VectorDiType = typename SolverTraitsType::VectorDiType;

  using ScalarType = typename SolverTraitsType::ScalarType;

  OutputVelocityAction(VectorDsType* maxVelocity)
    : _maxVelocity(maxVelocity) {}

  void
  setValue(CellAccessorType const& accessor) {
    int offset = -1;

    if (TDirection == 0) {
      offset = +1;
    }

    for (int d = 0; d < Dimensions; ++d) {
      if (d == TDimension) {
        if (TDirection == 0) {
          accessor.velocity(TDimension)
            = accessor.velocity(TDimension, +1, TDimension);
        } else {
          accessor.velocity(TDimension, -1, TDimension)
            = accessor.velocity(TDimension, -2, TDimension);
        }
      } else {
        accessor.velocity(d) = accessor.velocity(TDimension, offset, d);
      }
    }
  }

  VectorDsType* _maxVelocity;
};

template <typename TSolverTraits,
          int TDimension,
          int TDirection>
class InputFghAction {
public:
  using SolverTraitsType = TSolverTraits;

  enum {
    Dimensions = SolverTraitsType::Dimensions
  };

  using GridType = typename SolverTraitsType::GridType;

  using CellAccessorType = typename SolverTraitsType::CellAccessorType;

  using VectorDsType = typename SolverTraitsType::VectorDsType;

  using VectorDiType = typename SolverTraitsType::VectorDiType;

  using ScalarType = typename SolverTraitsType::ScalarType;

  InputFghAction(Configuration* parameters) : _configuration(parameters) {}

  void
  setValue(CellAccessorType const& accessor) {
    if (TDirection == 0) {
      accessor.fgh(TDimension)
        = _configuration->walls[TDimension][TDirection]->velocity()
            (TDimension);
    } else {
      accessor.fgh(TDimension, -1, TDimension)
        = _configuration->walls[TDimension][TDirection]->velocity()
            (TDimension);
    }
  }

  Configuration* _configuration;
};

template <typename TSolverTraits,
          int TDimension,
          int TDirection>
class ParabolicInputFghAction {
public:
  using SolverTraitsType = TSolverTraits;

  using CellAccessorType = typename SolverTraitsType::CellAccessorType;

  using VectorDsType = typename SolverTraitsType::VectorDsType;

  using ScalarType = typename SolverTraitsType::ScalarType;

  ParabolicInputFghAction(Configuration* parameters)
    : _configuration(parameters) {}

  void
  setValue(CellAccessorType const& accessor) {
    ScalarType result;
    result
      = _configuration->walls[TDimension][TDirection]->velocity()(TDimension);

    if (TDirection == 0) {
      compute_parabolic_input_velocity(accessor, TDimension, result);
      accessor.fgh(TDimension) = result;
    } else {
      compute_parabolic_input_velocity(
        accessor.relative(TDimension, -1),
        TDimension,
        result);
      accessor.fgh(TDimension, -1, TDimension) = result;
    }
  }

  Configuration* _configuration;
};

template <typename TSolverTraits,
          int TDimension,
          int TDirection>
class OutputFghAction {
public:
  using SolverTraitsType = TSolverTraits;

  enum {
    Dimensions = SolverTraitsType::Dimensions
  };

  using GridType = typename SolverTraitsType::GridType;

  using CellAccessorType = typename SolverTraitsType::CellAccessorType;

  using VectorDsType = typename SolverTraitsType::VectorDsType;

  using VectorDiType = typename SolverTraitsType::VectorDiType;

  using ScalarType = typename SolverTraitsType::ScalarType;

public:
  OutputFghAction() {}

  void
  setValue(CellAccessorType const& accessor) {
    if (TDirection == 0) {
      accessor.fgh(TDimension) = accessor.velocity(TDimension);
    } else {
      accessor.fgh(TDimension, -1, TDimension)
        = accessor.velocity(TDimension, -1, TDimension);
    }
  }
};
}
}
}
}
