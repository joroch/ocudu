// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "fapi_dummy_config.h"
#include "fapi_dummy_p5_gateway.h"
#include "fapi_dummy_p7_gateway.h"
#include "fapi_dummy_ue_simulator.h"
#include "ocudu/fapi/p7/p7_last_request_notifier.h"
#include "ocudu/fapi_adaptor/phy/p5/phy_fapi_p5_sector_adaptor.h"
#include "ocudu/fapi_adaptor/phy/p7/phy_fapi_p7_sector_adaptor.h"
#include "ocudu/fapi_adaptor/phy/phy_fapi_adaptor.h"
#include "ocudu/ran/slot_point_extended.h"
#include <atomic>
#include <limits>

namespace ocudu {

namespace fapi {
class error_indication_notifier;
class p7_indications_notifier;
class p7_slot_indication_notifier;
} // namespace fapi

namespace fapi_adaptor {

/// \brief Dummy FAPI sector adaptor.
///
/// Implements the P5 and P7 sector adaptor interfaces for a single cell.
/// P5 (param/config/start/stop) is fully handled.
/// P7: slot timing is driven by the shared fapi_dummy_timing_handler via on_new_slot().
/// UL PDUs are buffered and ACK'd positively (CRC ok, all HARQ bits = ack).
class fapi_dummy_sector : public phy_fapi_sector_adaptor,
                          public phy_fapi_p5_sector_adaptor,
                          public phy_fapi_p7_sector_adaptor
{
public:
  explicit fapi_dummy_sector(const fapi_dummy_cell_config& cfg);

  // phy_fapi_sector_adaptor
  void                        start() override {}
  void                        stop() override {}
  phy_fapi_p5_sector_adaptor& get_p5_sector_adaptor() override { return *this; }
  phy_fapi_p7_sector_adaptor& get_p7_sector_adaptor() override { return *this; }

  // phy_fapi_p5_sector_adaptor
  fapi::p5_requests_gateway& get_p5_requests_gateway() override { return p5_gw; }
  void set_p5_responses_notifier(fapi::p5_responses_notifier& n) override { p5_gw.set_notifier(n); }
  void set_error_indication_notifier(fapi::error_indication_notifier&) override {}

  // phy_fapi_p7_sector_adaptor
  fapi::p7_requests_gateway&      get_p7_requests_gateway() override { return p7_gw; }
  fapi::p7_last_request_notifier& get_p7_last_request_notifier() override { return p7_last_req; }
  void set_p7_slot_indication_notifier(fapi::p7_slot_indication_notifier& n) override
  {
    p7_gw.set_slot_indication_notifier(n);
  }
  void set_p7_indications_notifier(fapi::p7_indications_notifier& n) override { ue_sim.set_notifier(n); }

  /// Called by the timing handler once per slot.
  /// Fires the slot indication and processes any buffered UL PDUs for this slot.
  void on_new_slot(slot_point_extended slot);

  /// Credit-checked variant for the real-time timing handler.
  ///
  /// Increments the in-flight counter and calls on_new_slot() only when the number of
  /// outstanding slot indications is below max_in_flight (one less than the MAC's SPSC
  /// slot_ind queue capacity of 4).  Returns false without firing if the MAC is congested;
  /// the caller should back off before retrying.
  bool try_on_new_slot(slot_point_extended slot);

  /// Fires UL ACK responses (CRC/Rx_Data/UCI) for any UL_TTI stored at \p slot.
  ///
  /// Call this after the spin-wait confirms slot completion so that PUSCH/PUCCH PDUs
  /// sent by the MAC during slot processing are ACK'd with the correct UL slot.
  void flush_ul(slot_point slot) { ue_sim.flush_ul(slot); }

  /// Returns true if the MAC has signalled slot-completion for the given slot.
  /// Thread-safe: may be polled from the test main thread while the MAC runs on worker threads.
  bool is_slot_completed(slot_point slot) const
  {
    return p7_last_req.last_slot_val.load(std::memory_order_acquire) == slot.to_uint();
  }

private:
  /// P7 last-request notifier that records the most recently completed slot atomically
  /// and maintains the in-flight credit counter used by try_on_new_slot().
  ///
  /// on_last_message() is called from the MAC cell worker thread; last_slot_val,
  /// slots_in_flight, and mac_started are polled/modified from the test or timer executor thread.
  class p7_last_request_notifier_impl : public fapi::p7_last_request_notifier
  {
  public:
    void on_last_message(slot_point slot) override
    {
      // mac_started must be stored BEFORE last_slot_val so that callers spinning on
      // last_slot_val (via is_slot_completed()) are guaranteed to observe mac_started=true
      // via acquire-release ordering.
      mac_started.store(true, std::memory_order_release);
      last_slot_val.store(slot.to_uint(), std::memory_order_release);
      // Saturating decrement: only decrement if a credit was taken (try_on_new_slot path).
      // When on_new_slot() is called directly (test path), slots_in_flight stays at 0.
      uint32_t prev = slots_in_flight.load(std::memory_order_relaxed);
      if (prev > 0) {
        slots_in_flight.fetch_sub(1, std::memory_order_release);
      }
    }

    // Maximum number of slot indications that may be outstanding simultaneously.
    // Chosen as one less than the MAC's SPSC slot_ind queue capacity (4) for safety margin.
    static constexpr uint32_t max_in_flight = 3;

    std::atomic<uint32_t> last_slot_val{std::numeric_limits<uint32_t>::max()};
    std::atomic<uint32_t> slots_in_flight{0};
    std::atomic<bool>     mac_started{false};
  };

  fapi_dummy_cell_config          cfg;
  fapi_dummy_p5_gateway           p5_gw;
  fapi_dummy_p7_gateway           p7_gw;
  fapi_dummy_ue_simulator         ue_sim; // constructed with cfg.ue in the .cpp initialiser list
  p7_last_request_notifier_impl   p7_last_req;
};

} // namespace fapi_adaptor
} // namespace ocudu
