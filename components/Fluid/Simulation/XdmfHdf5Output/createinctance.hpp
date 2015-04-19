#pragma once

#include "Simulation/SolverTraits.hpp"

#include <memory>

namespace FsiSimulation {
namespace FluidSimulation {
class IterationResultWriter;
namespace XdmfHdf5Output {
template <typename TMemory>
std::unique_ptr<IterationResultWriter>
create_instance(TMemory const*);

extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <0, 0, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <0, 1, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <1, 0, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <1, 1, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <0, 0, double, 3>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <0, 1, double, 3>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <1, 0, double, 3>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename SfsfdSolverTraits
                <1, 1, double, 3>
                ::MemoryType const*);

extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <0, 0, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <0, 1, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <1, 0, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <1, 1, double, 2>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <0, 0, double, 3>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <0, 1, double, 3>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <1, 0, double, 3>
                ::MemoryType const*);
extern template std::unique_ptr<IterationResultWriter>
create_instance(typename IfsfdSolverTraits
                <1, 1, double, 3>
                ::MemoryType const*);
}
}
}
