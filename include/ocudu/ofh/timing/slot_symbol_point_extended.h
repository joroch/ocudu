/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/ofh/timing/slot_symbol_point.h"
#include "ocudu/ran/slot_point_extended.h"

namespace ocudu {
namespace ofh {

/// Represents the combined concept of a \c slot_point_extended and a symbol index inside a hyper SFN slot.
class slot_symbol_point_extended : private slot_symbol_point
{
public:
  slot_symbol_point_extended(slot_point_extended slot, unsigned symbol_index, unsigned nof_symbols_)
  {
    ocudu_assert(symbol_index < nof_symbols_, "Invalid symbol index");
    nof_symbols = nof_symbols_;
    numerology  = slot.numerology();
    count_val   = slot.count() * nof_symbols_ + symbol_index;
  }

  /// Takes a numerology, total count value and number of symbols.
  slot_symbol_point_extended(uint32_t numerology_, uint32_t count_, unsigned nof_symbols_)
  {
    ocudu_assert(numerology_ < NOF_NUMEROLOGIES, "Invalid numerology idx={} passed", unsigned(numerology));
    nof_symbols = nof_symbols_;
    numerology  = numerology_;
    count_val   = count_;
  }

  /// Slot point extended.
  slot_point_extended get_slot_extended() const
  {
    return is_valid() ? slot_point_extended(to_subcarrier_spacing(numerology), count_val / nof_symbols)
                      : slot_point_extended();
  }

  /// Returns the slot symbol point.
  slot_symbol_point get_slot_symbol_point() const
  {
    return is_valid()
               ? slot_symbol_point(get_numerology(), count_val % (get_nof_symbols_per_hyper_sfn()), get_nof_symbols())
               : slot_symbol_point();
  }

  using slot_symbol_point::get_nof_symbols;
  using slot_symbol_point::get_nof_symbols_per_hyper_sfn;
  using slot_symbol_point::get_nof_symbols_per_sfn;
  using slot_symbol_point::get_numerology;
  using slot_symbol_point::get_symbol_index;
  using slot_symbol_point::is_valid;

  /// Implementation of the sum operator, where \c jump is represented in number of symbols.
  slot_symbol_point_extended& operator+=(int jump)
  {
    const int nof_symbols_per_slot_wrap = NOF_HYPER_SFNS * get_nof_symbols_per_hyper_sfn();

    int tmp   = (static_cast<int>(count_val) + jump) % nof_symbols_per_slot_wrap;
    count_val = tmp + (tmp < 0 ? nof_symbols_per_slot_wrap : 0);
    return *this;
  }

  /// Implementation of the subtraction operator, where \c jump is represented in number of symbols.
  slot_symbol_point_extended& operator-=(int jump) { return this->operator+=(-jump); }

  /// Equality comparison of two slot_symbol_point_extended objects. Two slot symbol points are equal if their
  /// numerology, hyper-SFN, SFN slot index, symbol index and number of symbols have the same value.
  bool operator==(const slot_symbol_point_extended& other) const
  {
    return other.count_val == count_val && other.numerology == numerology && other.nof_symbols == nof_symbols;
  }

  /// Inequality comparison of two slot_symbol_point_extended objects.
  bool operator!=(const slot_symbol_point_extended& other) const { return !(*this == other); }

  /// Checks if "lhs" slot symbol point extended is lower than "rhs". It assumes that both "lhs" and "rhs" use the same
  /// numerology and number of symbols. This comparison accounts for the wrap-around of slot index, SFNs and hyper-SFNs.
  /// The ambiguity range of the comparison is equal to half of the total number of slot symbol points in all
  /// hyperframe range.
  bool operator<(const slot_symbol_point_extended& other) const
  {
    ocudu_assert(numerology == other.numerology, "Comparing slots symbol point of different numerologies");
    ocudu_assert(get_nof_symbols() == other.get_nof_symbols(),
                 "Comparing slots symbol point with different number of symbols");

    const int nof_symbols_per_slot_wrap = NOF_HYPER_SFNS * get_nof_symbols_per_hyper_sfn();

    int v = static_cast<int>(other.count_val) - static_cast<int>(count_val);
    if (v > 0) {
      return (v < nof_symbols_per_slot_wrap / 2);
    }
    return (v < -nof_symbols_per_slot_wrap / 2);
  }

  /// Other lower/higher comparisons that build on top of operator== and operator<.
  bool operator<=(const slot_symbol_point_extended& other) const { return (*this == other) || (*this < other); }
  bool operator>=(const slot_symbol_point_extended& other) const { return !(*this < other); }
  bool operator>(const slot_symbol_point_extended& other) const { return (*this != other) && *this >= other; }

  uint32_t raw_value() const { return count_val; }
};

inline slot_symbol_point_extended operator+(slot_symbol_point_extended symbol_point, int jump)
{
  symbol_point += jump;
  return symbol_point;
}

inline slot_symbol_point_extended operator+(int jump, slot_symbol_point_extended symbol_point)
{
  symbol_point += jump;
  return symbol_point;
}

inline slot_symbol_point_extended operator-(slot_symbol_point_extended symbol_point, int jump)
{
  symbol_point -= jump;
  return symbol_point;
}

inline slot_symbol_point_extended operator-(int jump, slot_symbol_point_extended symbol_point)
{
  symbol_point -= jump;
  return symbol_point;
}

/// Implementation of subtraction operation.
/// Returns difference between two slot_symbol_point objects in number of symbols.
inline int operator-(slot_symbol_point_extended lhs, slot_symbol_point_extended rhs)
{
  ocudu_assert(rhs.get_numerology() == lhs.get_numerology(),
               "Cannot calculate the distance of two slot symbol points that have different numerologies");
  ocudu_assert(rhs.get_nof_symbols() == lhs.get_nof_symbols(),
               "Cannot calculate the distance of two slot symbol points that have a different number of symbols");

  const int nof_symbols_per_slot_wrap = NOF_HYPER_SFNS * lhs.get_nof_symbols_per_hyper_sfn();

  int tmp = static_cast<int>(lhs.raw_value()) - static_cast<int>(rhs.raw_value());
  if (tmp > (nof_symbols_per_slot_wrap / 2)) {
    return (tmp - nof_symbols_per_slot_wrap);
  }

  if (tmp < -(nof_symbols_per_slot_wrap / 2)) {
    return tmp + nof_symbols_per_slot_wrap;
  }
  return tmp;
}

} // namespace ofh
} // namespace ocudu
