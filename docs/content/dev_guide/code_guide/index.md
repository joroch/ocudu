<a id="code-guide-overview"></a>

# Code Style Guide

This document establishes the basic guidelines and recommendations for C and C++ programming styles within srsRAN Project code base. Many of the guidelines are C++ specific, however
some can still be applied to C. The main goal of this document is to help establish a consistent programming style throughout srsRAN Project code base, and
improve the readability and maintainability of code committed by users.

This document is heavily inspired in [LLVM’s coding standards](https://llvm.org/docs/CodingStandards.html), please refer to this
if you would like to take a further look into coding standards and best practices.

---

**Contents:**

* [Language and libraries](1_language_libraries.md)
  * [C & C++ version](1_language_libraries.md#c-c-version)
  * [Use of the C++ standard library](1_language_libraries.md#use-of-the-c-standard-library)
* [Mechanical source aspects](2_mechanical_source_aspects.md)
  * [Source Code Formatting](2_mechanical_source_aspects.md#source-code-formatting)
  * [Comments](2_mechanical_source_aspects.md#comments)
  * [Header Guards](2_mechanical_source_aspects.md#header-guards)
  * [#include Style](2_mechanical_source_aspects.md#include-style)
  * [Language and compiler aspects](2_mechanical_source_aspects.md#language-and-compiler-aspects)
* [Style Aspects: High Level Issues](3_style_aspects_high_level.md)
  * [Self-contained Headers](3_style_aspects_high_level.md#self-contained-headers)
  * [Using `#include` Sparingly](3_style_aspects_high_level.md#using-include-sparingly)
  * [Using “Internal” Headers](3_style_aspects_high_level.md#using-internal-headers)
  * [Use of `namespace`](3_style_aspects_high_level.md#use-of-namespace)
  * [Using “early exits” and `continue`](3_style_aspects_high_level.md#using-early-exits-and-continue)
  * [Avoid `else` after a `return` statement](3_style_aspects_high_level.md#avoid-else-after-a-return-statement)
  * [Use of Static Helper Functions](3_style_aspects_high_level.md#use-of-static-helper-functions)
* [Style Aspects: Low Level Issues](4_style_aspects_low_level.md)
  * [Naming Conventions](4_style_aspects_low_level.md#naming-conventions)
  * [Assert and expect](4_style_aspects_low_level.md#assert-and-expect)
  * [Do not use `using namespace std`](4_style_aspects_low_level.md#do-not-use-using-namespace-std)
  * [Using Range for Loops](4_style_aspects_low_level.md#using-range-for-loops)
  * [Loop Structure](4_style_aspects_low_level.md#loop-structure)
  * [Using Pre-increment](4_style_aspects_low_level.md#using-pre-increment)
  * [Use of Anonymous Namespaces](4_style_aspects_low_level.md#use-of-anonymous-namespaces)
  * [Using C++ Casts](4_style_aspects_low_level.md#using-c-casts)
* [Recommendations](5_recommendations.md)
  * [Function and Class Length](5_recommendations.md#function-and-class-length)
  * [Scope](5_recommendations.md#scope)
  * [Logical Operators](5_recommendations.md#logical-operators)
  * [Using References(&) Over Pointers(\*)](5_recommendations.md#using-references-over-pointers)
  * [Const Correctness](5_recommendations.md#const-correctness)
  * [Avoid Complex Expressions](5_recommendations.md#avoid-complex-expressions)
  * [Magic numbers](5_recommendations.md#magic-numbers)
  * [Fixed Width Integer Types](5_recommendations.md#fixed-width-integer-types)
  * [Function signatures](5_recommendations.md#function-signatures)
  * [Class Layout Example](5_recommendations.md#class-layout-example)
* [Self Generating Documentation](6_self_generating_docs.md)
  * [General Aspects](6_self_generating_docs.md#general-aspects)
  * [Files](6_self_generating_docs.md#files)
  * [Classes and Structures](6_self_generating_docs.md#classes-and-structures)
  * [Class Methods and Free Functions](6_self_generating_docs.md#class-methods-and-free-functions)
  * [Class Data Members, Objects, Variables](6_self_generating_docs.md#class-data-members-objects-variables)
* [Commit Formatting](7_commit_formatting.md)
