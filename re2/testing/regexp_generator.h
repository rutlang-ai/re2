// Copyright 2008 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_TESTING_REGEXP_GENERATOR_H_
#define RE2_TESTING_REGEXP_GENERATOR_H_

// Regular expression generator: generates all possible
// regular expressions within given parameters (see below for details).

#include <stdint.h>

#include <random>
#include <string>
#include <vector>

#include "re2/testing/support.h"

namespace re2 {

// Regular expression generator.
//
// Given a set of atom expressions like "a", "b", or "."
// and operators like "%s*", generates all possible regular expressions
// using at most maxbases base expressions and maxops operators.
// For each such expression re, calls HandleRegexp(re).
//
// Callers are expected to subclass RegexpGenerator and provide HandleRegexp.
//
class RegexpGenerator {
 public:
  RegexpGenerator(int maxatoms, int maxops,
                  const re2::Vector<re2::String>& atoms,
                  const re2::Vector<re2::String>& ops);
  virtual ~RegexpGenerator() {}

  // Generates all the regular expressions, calling HandleRegexp(re) for each.
  void Generate();

  // Generates n random regular expressions, calling HandleRegexp(re) for each.
  void GenerateRandom(int32_t seed, int n);

  // Handles a regular expression.  Must be provided by subclass.
  virtual void HandleRegexp(const re2::String& regexp) = 0;

  // The egrep regexp operators: * + ? | and concatenation.
  static const re2::Vector<re2::String>& EgrepOps();

 private:
  void RunPostfix(const re2::Vector<re2::String>& post);
  void GeneratePostfix(re2::Vector<re2::String>* post,
                       int nstk, int ops, int lits);
  bool GenerateRandomPostfix(re2::Vector<re2::String>* post,
                             int nstk, int ops, int lits);

  int maxatoms_;                    // Maximum number of atoms allowed in expr.
  int maxops_;                      // Maximum number of ops allowed in expr.
  re2::Vector<re2::String> atoms_;  // Possible atoms.
  re2::Vector<re2::String> ops_;    // Possible ops.
  std::minstd_rand0 rng_;           // Random number generator.

  RegexpGenerator(const RegexpGenerator&) = delete;
  RegexpGenerator& operator=(const RegexpGenerator&) = delete;
};

// Helpers for preparing arguments to RegexpGenerator constructor.

// Returns one string for each character in s.
re2::Vector<re2::String> Explode(re2::StringView s);

// Splits string everywhere sep is found, returning
// vector of pieces.
re2::Vector<re2::String> Split(re2::StringView sep, re2::StringView s);

}  // namespace re2

#endif  // RE2_TESTING_REGEXP_GENERATOR_H_
