// Copyright 2008 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Exhaustive testing of regular expression matching.

// Each test picks an alphabet (e.g., "abc"), a maximum string length,
// a maximum regular expression length, and a maximum number of letters
// that can appear in the regular expression.  Given these parameters,
// it tries every possible regular expression and string, verifying that
// the NFA, DFA, and a trivial backtracking implementation agree about
// the location of the match.

#include "re2/testing/exhaustive_tester.h"

#include <stdio.h>

#include <string>
#include <vector>

#include "re2/testing/support.h"
#include "gtest/gtest.h"
#include "re2/prog.h"
#include "re2/re2.h"
#include "re2/testing/regexp_generator.h"
#include "re2/testing/tester.h"

// For target `log' in the Makefile.
#ifndef LOGGING
#define LOGGING 0
#endif

RE2_FLAG(bool, show_regexps, false, "show regexps during testing");

RE2_FLAG(int, max_bad_regexp_inputs, 1,
          "Stop testing a regular expression after finding this many "
          "strings that break it.");

namespace re2 {

static char* escape(re2::StringView sp) {
  static char buf[512];
  char* p = buf;
  *p++ = '\"';
  for (size_t i = 0; i < sp.size(); i++) {
    if(p+5 >= buf+sizeof buf)
      RE2_LOG(FATAL) << "ExhaustiveTester escape: too long";
    if(sp[i] == '\\' || sp[i] == '\"') {
      *p++ = '\\';
      *p++ = sp[i];
    } else if(sp[i] == '\n') {
      *p++ = '\\';
      *p++ = 'n';
    } else {
      *p++ = sp[i];
    }
  }
  *p++ = '\"';
  *p = '\0';
  return buf;
}

static void PrintResult(const RE2& re, re2::StringView input,
                        RE2::Anchor anchor, re2::StringView* m, int n) {
  if (!re.Match(input, 0, input.size(), anchor, m, n)) {
    re2::internal::PrintF("-");
    return;
  }
  for (int i = 0; i < n; i++) {
    if (i > 0)
      re2::internal::PrintF(" ");
    if (m[i].data() == NULL)
      re2::internal::PrintF("-");
    else
      re2::internal::PrintF("%d-%d",
                   BeginPtr(m[i]) - BeginPtr(input),
                   EndPtr(m[i]) - BeginPtr(input));
  }
}

// Processes a single generated regexp.
// Compiles it using Regexp interface and PCRE, and then
// checks that NFA, DFA, and PCRE all return the same results.
void ExhaustiveTester::HandleRegexp(const re2::String& const_regexp) {
  regexps_++;
  re2::String regexp = const_regexp;
  if (!topwrapper_.empty()) {
    auto fmt = re2::internal::ParsedFormat<'s'>::New(topwrapper_);
    RE2_CHECK(fmt != nullptr);
    regexp = re2::internal::StrFormat(*fmt, regexp);
  }

  if (re2::internal::GetFlag(FLAGS_show_regexps)) {
    re2::internal::PrintF("\r%s", regexp);
    fflush(stdout);
  }

  if (LOGGING) {
    // Write out test cases and answers for use in testing
    // other implementations, such as Go's regexp package.
    if (randomstrings_)
      RE2_LOG(ERROR) << "Cannot log with random strings.";
    if (regexps_ == 1) {  // first
      re2::internal::PrintF("strings\n");
      strgen_.Reset();
      while (strgen_.HasNext())
        re2::internal::PrintF("%s\n", escape(strgen_.Next()));
      re2::internal::PrintF("regexps\n");
    }
    re2::internal::PrintF("%s\n", escape(regexp));

    RE2 re(regexp);
    RE2::Options longest;
    longest.set_longest_match(true);
    RE2 relongest(regexp, longest);
    int ngroup = re.NumberOfCapturingGroups()+1;
    re2::StringView* group = new re2::StringView[ngroup];

    strgen_.Reset();
    while (strgen_.HasNext()) {
      re2::StringView input = strgen_.Next();
      PrintResult(re, input, RE2::ANCHOR_BOTH, group, ngroup);
      re2::internal::PrintF(";");
      PrintResult(re, input, RE2::UNANCHORED, group, ngroup);
      re2::internal::PrintF(";");
      PrintResult(relongest, input, RE2::ANCHOR_BOTH, group, ngroup);
      re2::internal::PrintF(";");
      PrintResult(relongest, input, RE2::UNANCHORED, group, ngroup);
      re2::internal::PrintF("\n");
    }
    delete[] group;
    return;
  }

  Tester tester(regexp);
  if (tester.error())
    return;

  strgen_.Reset();
  strgen_.GenerateNULL();
  if (randomstrings_)
    strgen_.Random(stringseed_, stringcount_);
  int bad_inputs = 0;
  while (strgen_.HasNext()) {
    tests_++;
    if (!tester.TestInput(strgen_.Next())) {
      failures_++;
      if (++bad_inputs >= re2::internal::GetFlag(FLAGS_max_bad_regexp_inputs))
        break;
    }
  }
}

// Runs an exhaustive test on the given parameters.
void ExhaustiveTest(int maxatoms, int maxops,
                    const re2::Vector<re2::String>& alphabet,
                    const re2::Vector<re2::String>& ops,
                    int maxstrlen,
                    const re2::Vector<re2::String>& stralphabet,
                    const re2::String& wrapper,
                    const re2::String& topwrapper) {
  if (RE2_DEBUG_MODE) {
    if (maxatoms > 1)
      maxatoms--;
    if (maxops > 1)
      maxops--;
    if (maxstrlen > 1)
      maxstrlen--;
  }
  ExhaustiveTester t(maxatoms, maxops, alphabet, ops,
                     maxstrlen, stralphabet, wrapper,
                     topwrapper);
  t.Generate();
  if (!LOGGING) {
    re2::internal::PrintF("%d regexps, %d tests, %d failures [%d/%d str]\n",
                 t.regexps(), t.tests(), t.failures(), maxstrlen, stralphabet.size());
  }
  EXPECT_EQ(0, t.failures());
}

// Runs an exhaustive test using the given parameters and
// the basic egrep operators.
void EgrepTest(int maxatoms, int maxops, const re2::String& alphabet,
               int maxstrlen, const re2::String& stralphabet,
               const re2::String& wrapper) {
  const char* tops[] = { "", "^(?:%s)", "(?:%s)$", "^(?:%s)$" };

  for (size_t i = 0; i < RE2_ARRAYSIZE(tops); i++) {
    ExhaustiveTest(maxatoms, maxops,
                   Split("", alphabet),
                   RegexpGenerator::EgrepOps(),
                   maxstrlen,
                   Split("", stralphabet),
                   wrapper,
                   tops[i]);
  }
}

}  // namespace re2
