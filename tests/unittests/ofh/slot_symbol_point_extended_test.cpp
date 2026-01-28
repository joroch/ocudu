/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/ofh/timing/slot_symbol_point_extended.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ofh;

TEST(slot_symbol_point_extended_test, basic_construction_and_getters_should_pass)
{
  const unsigned             nof_symbols    = 14;
  const unsigned             initial_symbol = 7;
  slot_point_extended        slot(subcarrier_spacing::kHz30, 20, 4, 1);
  slot_symbol_point_extended symbol_point(slot, initial_symbol, nof_symbols);

  ASSERT_EQ(symbol_point.get_nof_symbols(), nof_symbols);
  ASSERT_EQ(symbol_point.get_slot_extended(), slot);
  ASSERT_EQ(symbol_point.get_symbol_index(), initial_symbol);
}

TEST(slot_symbol_point_extended_test, test_wraparound)
{
  slot_point_extended        slot(subcarrier_spacing::kHz30, 1023, 1023, 19);
  slot_symbol_point_extended symbol_point(slot, 13, 14);
  symbol_point += 1;
  ++slot;

  ASSERT_EQ(symbol_point.get_slot_extended(), slot);
  ASSERT_EQ(symbol_point.get_symbol_index(), 0);
}

TEST(slot_symbol_point_extended_test, symbol_add_and_subtract_should_pass)
{
  const unsigned           nof_symbols    = 14;
  const unsigned           initial_symbol = 7;
  const subcarrier_spacing scs            = subcarrier_spacing::kHz30;

  slot_point_extended        slot(scs, 20, 0, 0);
  slot_symbol_point_extended symbol_point(slot, initial_symbol, nof_symbols);

  symbol_point += 7;
  ASSERT_EQ(symbol_point.get_symbol_index(), 0);
  ASSERT_EQ(symbol_point.get_slot_extended(), slot_point_extended(scs, 20, 0, 1));

  symbol_point += 30;
  ASSERT_EQ(symbol_point.get_symbol_index(), 2);
  ASSERT_EQ(symbol_point.get_slot_extended(), slot_point_extended(scs, 20, 0, 3));

  symbol_point -= 30;
  ASSERT_EQ(symbol_point.get_symbol_index(), 0);
  ASSERT_EQ(symbol_point.get_slot_extended(), slot_point_extended(scs, 20, 0, 1));

  symbol_point -= 7;
  ASSERT_EQ(symbol_point.get_symbol_index(), initial_symbol);
  ASSERT_EQ(symbol_point.get_slot_extended(), slot);

  // Test wrap around.
  slot         = slot_point_extended(scs, 0);
  symbol_point = slot_symbol_point_extended(slot, 0, nof_symbols);
  symbol_point -= 7;
  ASSERT_EQ(symbol_point.get_symbol_index(), 7);
  ASSERT_EQ(symbol_point.get_slot_extended(), slot_point_extended(scs, 1023, 1023, 19));
}

TEST(slot_symbol_point_extended_test, equal_slot_symbol_points)
{
  const unsigned             nof_symbols   = 14;
  const unsigned             numerology    = 1;
  const unsigned             initial_value = 7;
  slot_symbol_point_extended symbol_point(numerology, initial_value, nof_symbols);
  slot_symbol_point_extended symbol_point2(numerology, initial_value, nof_symbols);

  ASSERT_EQ(symbol_point, symbol_point2);
}

TEST(slot_symbol_point_extended_test, different_slot_symbol_points)
{
  const unsigned             nof_symbols   = 14;
  const unsigned             numerology    = 1;
  const unsigned             initial_value = 7;
  slot_symbol_point_extended symbol_point(numerology, initial_value, nof_symbols);
  slot_symbol_point_extended symbol_point2(numerology, initial_value + 1, nof_symbols);

  ASSERT_NE(symbol_point, symbol_point2);
}

TEST(slot_symbol_point_extended_test, smaller_slot_symbol_point)
{
  const unsigned             nof_symbols   = 14;
  const unsigned             numerology    = 1;
  const unsigned             initial_value = 7;
  slot_symbol_point_extended symbol_point(numerology, initial_value, nof_symbols);
  slot_symbol_point_extended symbol_point2(numerology, initial_value + 1, nof_symbols);

  ASSERT_TRUE(symbol_point < symbol_point2);
}

TEST(slot_symbol_point_extended_test, smaller_slot_symbol_point_with_slot_constructor)
{
  const unsigned             nof_symbols    = 14;
  const unsigned             initial_symbol = 7;
  const subcarrier_spacing   scs            = subcarrier_spacing::kHz30;
  slot_point_extended        slot(scs, 20, 0, 0);
  slot_point_extended        slot2(scs, 21, 0, 0);
  slot_symbol_point_extended symbol_point(slot, initial_symbol, nof_symbols);
  slot_symbol_point_extended symbol_point2(slot2, initial_symbol, nof_symbols);

  ASSERT_TRUE(symbol_point < symbol_point2);
}

TEST(slot_symbol_point_extended_test, smaller_slot_symbol_point_same_slot_different_symbol)
{
  const unsigned             nof_symbols    = 14;
  const unsigned             initial_symbol = 7;
  const subcarrier_spacing   scs            = subcarrier_spacing::kHz30;
  slot_point_extended        slot(scs, 20, 0, 0);
  slot_symbol_point_extended symbol_point(slot, initial_symbol, nof_symbols);
  slot_symbol_point_extended symbol_point2(slot, initial_symbol + 1, nof_symbols);

  ASSERT_TRUE(symbol_point < symbol_point2);
}

TEST(slot_symbol_point_extended_test, symbols_subtraction_should_pass)
{
  const unsigned      nof_symbols    = 14;
  const unsigned      numerology     = 1;
  const unsigned      initial_symbol = 7;
  slot_point_extended slot(to_subcarrier_spacing(numerology), 20, 0, 0);

  int                        distance = 10;
  slot_symbol_point_extended symbol_point_0(slot, initial_symbol, nof_symbols);
  slot_symbol_point_extended symbol_point_1 = symbol_point_0 + distance;

  int calculated_distance = symbol_point_1 - symbol_point_0;
  ASSERT_EQ(calculated_distance, distance);
  calculated_distance = symbol_point_0 - symbol_point_1;
  ASSERT_EQ(-calculated_distance, distance);

  // Test wrap around.
  distance            = -21;
  slot                = slot_point_extended(to_subcarrier_spacing(numerology), 0);
  symbol_point_0      = slot_symbol_point_extended(slot, 0, nof_symbols);
  symbol_point_1      = symbol_point_0 + distance;
  calculated_distance = symbol_point_1 - symbol_point_0;
  ASSERT_EQ(calculated_distance, distance);
  calculated_distance = symbol_point_0 - symbol_point_1;
  ASSERT_EQ(-calculated_distance, distance);
}
