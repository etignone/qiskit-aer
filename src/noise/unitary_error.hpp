/**
 * Copyright 2018, IBM.
 *
 * This source code is licensed under the Apache License, Version 2.0 found in
 * the LICENSE.txt file in the root directory of this source tree.
 */

/**
 * @file    unitary_error.hpp
 * @brief   Unitary Error class for Qiskit-Aer simulator
 * @author  Christopher J. Wood <cjwood@us.ibm.com>
 */

#ifndef _aer_noise_unitary_error_hpp_
#define _aer_noise_unitary_error_hpp_

#include "noise/abstract_error.hpp"

// TODO optimization for gate fusion with original operator matrix

namespace AER {
namespace Noise {

//=========================================================================
// Unitary Error class
//=========================================================================

// Unitary error class that can model mixed unitary error channels, and
// coherent unitary errors.

class UnitaryError : public AbstractError {
public:
  //-----------------------------------------------------------------------
  // Error base required methods
  //-----------------------------------------------------------------------

  // Sample a noisy implementation of op
  NoiseOps sample_noise(const reg_t &qubits,
                        RngEngine &rng) override;

  //-----------------------------------------------------------------------
  // Additional class methods
  //-----------------------------------------------------------------------

  // Sets the probabilities for the given error matrices. The length
  // of the probability vector should be less than or equal to the number
  // of matrices. If the total of the vector is less than 1, the remaining
  // probability 1 - total is the probability of no error (identity matrix)
  void set_probabilities(const rvector_t &probs);

  // Sets the unitary error matrices for the error map
  void set_unitaries(const std::vector<cmatrix_t> &mats);

  // Set the sampled errors to be applied after the original operation
  inline void set_errors_after() {errors_after_op_ = true;}

  // Set the sampled errors to be applied before the original operation
  inline void set_errors_before() {errors_after_op_ = false;}

  // Set whether the input operator should be combined into the error
  // term (if compatible). The default is True
  inline void combine_error(bool val) {combine_error_ = val;}

protected:
  // Probabilities, first entry is no-error (identity)
  std::discrete_distribution<uint_t> probabilities_;

  // List of unitary error matrices
  std::vector<cmatrix_t> unitaries_;

  // Flag for applying errors before or after the operation
  bool errors_after_op_ = true; 

  // Combine error with input matrix to return a single operation
  // if it acts on the same qubits
  bool combine_error_ = true;
};

//-------------------------------------------------------------------------
// Implementation: Mixed unitary error subclass
//-------------------------------------------------------------------------

UnitaryError::NoiseOps UnitaryError::sample_noise(const reg_t &qubits,
                                                  RngEngine &rng) {
  auto r = rng.rand_int(probabilities_);
  // Check for no error case
  if (r == 0) {
    // no error
    return {};
  }
  // Check for invalid arguments
  if (r > unitaries_.size()) {
    throw std::invalid_argument("Unitary error probability vector does not match number of unitaries.");
  }
  if (unitaries_.empty()) {
    throw std::invalid_argument("Unitary error matrices are not set.");
  }
  // Get the error operation for the unitary
  Operations::Op error;
  error.name = "mat";
  error.mats.push_back(unitaries_[r - 1]);
  error.qubits = qubits;
  return {error};
}


void UnitaryError::set_probabilities(const rvector_t &probs) {
  rvector_t probs_with_identity({1.});
  bool probs_valid = true;
  for (const auto &p : probs) {
    probs_valid &= !(p < 0 || p > 1);
    probs_with_identity[0] -= p;
    probs_with_identity.push_back(p);
  }
  if (probs_with_identity[0] > 1 || probs_with_identity[0] < 0 || !probs_valid) {
    throw std::invalid_argument("UnitaryError: invalid probability vector.");
  }
  probabilities_ = std::discrete_distribution<uint_t>(
                      probs_with_identity.begin(),
                      probs_with_identity.end());
}

void UnitaryError::set_unitaries(const std::vector<cmatrix_t> &mats) {
  unitaries_ = mats;
}

//-------------------------------------------------------------------------
} // end namespace Noise
//-------------------------------------------------------------------------
} // end namespace AER
//-------------------------------------------------------------------------
#endif