## New submission following email from Brian Ripley reporting presence of `std::terminate`
in code-based, but possibly not compiled.

* `std::exit` was present in an _unused_ .cpp file; this file is now removed.

* We also removed dependence on packages `bit` (orphaned), `ffbase` and `ff`

## Test environments
* local OS X install, R 4.0.0
* ubuntu 14.04 (on travis-ci), R 3.6.3, gcc 4.8.4 and gcc 6.0
* win-builder (devel and release)

## R CMD check results
* There were no ERRORs or WARNINGs
* There is 1 occasional NOTE:
  checking installed package size ... NOTE
    installed size is 22.5Mb
    sub-directories of 1Mb or more:
      libs 21.7Mb

This occurs on systems (like `r-devel-linux-x86_64-fedora-clang`) that include debug
symbols in their compilation; Cyclops performance is heavily dependent on many template
instantiations that generate a large library when including debug symbols.  Future
availability of C++17 `if (constexpr ...)` should decrease library size substantially.

## Downstream dependencies
There are currently no downstream dependencies.
