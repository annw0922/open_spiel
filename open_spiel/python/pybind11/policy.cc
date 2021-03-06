// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "open_spiel/python/pybind11/policy.h"

// Python bindings for policies and algorithms handling them.

#include "open_spiel/algorithms/best_response.h"
#include "open_spiel/algorithms/cfr.h"
#include "open_spiel/algorithms/cfr_br.h"
#include "open_spiel/algorithms/deterministic_policy.h"
#include "open_spiel/algorithms/expected_returns.h"
#include "open_spiel/algorithms/is_mcts.h"
#include "open_spiel/algorithms/mcts.h"
#include "open_spiel/algorithms/tabular_exploitability.h"
#include "open_spiel/policy.h"
#include "open_spiel/spiel.h"
#include "pybind11/include/pybind11/detail/common.h"
#include "pybind11/include/pybind11/detail/descr.h"
#include "pybind11/include/pybind11/functional.h"
#include "pybind11/include/pybind11/numpy.h"
#include "pybind11/include/pybind11/operators.h"
#include "pybind11/include/pybind11/pybind11.h"
#include "pybind11/include/pybind11/stl.h"

namespace open_spiel {
namespace {

using ::open_spiel::algorithms::Exploitability;
using ::open_spiel::algorithms::NashConv;
using ::open_spiel::algorithms::TabularBestResponse;

namespace py = ::pybind11;
}  // namespace

void init_pyspiel_policy(py::module& m) {
  py::class_<TabularBestResponse>(m, "TabularBestResponse")
      .def(py::init<const open_spiel::Game&, int,
                    const std::unordered_map<std::string,
                                             open_spiel::ActionsAndProbs>&>())
      .def(py::init<const open_spiel::Game&, int, const open_spiel::Policy*>())
      .def("value", &TabularBestResponse::Value)
      .def("get_best_response_policy",
           &TabularBestResponse::GetBestResponsePolicy)
      .def("get_best_response_actions",
           &TabularBestResponse::GetBestResponseActions)
      .def("set_policy", py::overload_cast<const std::unordered_map<
                             std::string, open_spiel::ActionsAndProbs>&>(
                             &TabularBestResponse::SetPolicy))
      .def("set_policy",
           py::overload_cast<const Policy*>(&TabularBestResponse::SetPolicy));

  py::class_<open_spiel::Policy>(m, "Policy")
      .def("action_probabilities",
           (std::unordered_map<Action, double>(open_spiel::Policy::*)(
               const open_spiel::State&) const) &
               open_spiel::Policy::GetStatePolicyAsMap)
      .def("get_state_policy", (ActionsAndProbs(open_spiel::Policy::*)(
                                   const open_spiel::State&) const) &
                                   open_spiel::Policy::GetStatePolicy)
      .def("get_state_policy_as_map",
           (std::unordered_map<Action, double>(open_spiel::Policy::*)(
               const std::string&) const) &
               open_spiel::Policy::GetStatePolicyAsMap);

  // A tabular policy represented internally as a map. Note that this
  // implementation is not directly compatible with the Python TabularPolicy
  // implementation; the latter is implemented as a table of size
  // [num_states, num_actions], while this is implemented as a map. It is
  // non-trivial to convert between the two, but we have a function that does so
  // in the open_spiel/python/policy.py file.
  py::class_<open_spiel::TabularPolicy, open_spiel::Policy>(m, "TabularPolicy")
      .def(py::init<const std::unordered_map<std::string, ActionsAndProbs>&>())
      .def("get_state_policy", &open_spiel::TabularPolicy::GetStatePolicy)
      .def("policy_table",
           py::overload_cast<>(&open_spiel::TabularPolicy::PolicyTable));

  m.def("UniformRandomPolicy", &open_spiel::GetUniformPolicy);
  py::class_<open_spiel::UniformPolicy, open_spiel::Policy>(m, "UniformPolicy")
      .def(py::init<>())
      .def("get_state_policy", &open_spiel::UniformPolicy::GetStatePolicy);

  py::class_<open_spiel::algorithms::CFRSolver>(m, "CFRSolver")
      .def(py::init<const Game&>())
      .def("evaluate_and_update_policy",
           &open_spiel::algorithms::CFRSolver::EvaluateAndUpdatePolicy)
      .def("current_policy", &open_spiel::algorithms::CFRSolver::CurrentPolicy)
      .def("average_policy", &open_spiel::algorithms::CFRSolver::AveragePolicy);

  py::class_<open_spiel::algorithms::CFRPlusSolver>(m, "CFRPlusSolver")
      .def(py::init<const Game&>())
      .def("evaluate_and_update_policy",
           &open_spiel::algorithms::CFRPlusSolver::EvaluateAndUpdatePolicy)
      .def("current_policy", &open_spiel::algorithms::CFRSolver::CurrentPolicy)
      .def("average_policy",
           &open_spiel::algorithms::CFRPlusSolver::AveragePolicy);

  py::class_<open_spiel::algorithms::CFRBRSolver>(m, "CFRBRSolver")
      .def(py::init<const Game&>())
      .def("evaluate_and_update_policy",
           &open_spiel::algorithms::CFRPlusSolver::EvaluateAndUpdatePolicy)
      .def("current_policy", &open_spiel::algorithms::CFRSolver::CurrentPolicy)
      .def("average_policy",
           &open_spiel::algorithms::CFRPlusSolver::AveragePolicy);

  m.def("expected_returns",
        py::overload_cast<const State&, const std::vector<const Policy*>&, int,
                          bool>(&open_spiel::algorithms::ExpectedReturns),
        "Computes the undiscounted expected returns from a depth-limited "
        "search.");

  m.def("exploitability",
        py::overload_cast<const Game&, const Policy&>(&Exploitability),
        "Returns the sum of the utility that a best responder wins when when "
        "playing against 1) the player 0 policy contained in `policy` and 2) "
        "the player 1 policy contained in `policy`."
        "This only works for two player, zero- or constant-sum sequential "
        "games, and raises a SpielFatalError if an incompatible game is passed "
        "to it.");

  m.def(
      "exploitability",
      py::overload_cast<
          const Game&, const std::unordered_map<std::string, ActionsAndProbs>&>(
          &Exploitability),
      "Returns the sum of the utility that a best responder wins when when "
      "playing against 1) the player 0 policy contained in `policy` and 2) "
      "the player 1 policy contained in `policy`."
      "This only works for two player, zero- or constant-sum sequential "
      "games, and raises a SpielFatalError if an incompatible game is passed "
      "to it.");

  m.def("nash_conv", py::overload_cast<const Game&, const Policy&>(&NashConv),
        "Returns the sum of the utility that a best responder wins when when "
        "playing against 1) the player 0 policy contained in `policy` and 2) "
        "the player 1 policy contained in `policy`."
        "This only works for two player, zero- or constant-sum sequential "
        "games, and raises a SpielFatalError if an incompatible game is passed "
        "to it.");

  m.def(
      "nash_conv",
      py::overload_cast<
          const Game&, const std::unordered_map<std::string, ActionsAndProbs>&>(
          &NashConv),
      "Calculates a measure of how far the given policy is from a Nash "
      "equilibrium by returning the sum of the improvements in the value "
      "that each player could obtain by unilaterally changing their strategy "
      "while the opposing player maintains their current strategy (which "
      "for a Nash equilibrium, this value is 0).");

  m.def("num_deterministic_policies",
        open_spiel::algorithms::NumDeterministicPolicies,
        "Returns number of determinstic policies in this game for a player, "
        "or -1 if there are more than 2^64 - 1 policies.");
}
}  // namespace open_spiel
