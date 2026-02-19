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

#include "logical_channel_system.h"
#include "ta_management_system.h"
#include "ue_cell.h"
#include "ue_drx_controller.h"
#include "ocudu/ran/du_types.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/scheduler/mac_scheduler.h"
#include "ocudu/scheduler/scheduler_feedback_handler.h"

namespace ocudu {

/// \brief Structure to lookup UE cells based on DU cell index or UE-specific cell index.
struct ue_cell_lookup {
  /// List of UE cells indexed by \c du_cell_index_t.
  slotted_id_table<du_cell_index_t, ue_cell*, MAX_NOF_DU_CELLS> du_cells;
  /// List of UE cells indexed by \c ue_cell_index_t. The size of the list is equal to the number of cells aggregated
  /// and configured for the UE. PCell corresponds to ue_cell_index=0. the first SCell corresponds to ue_cell_index=1,
  /// etc.
  static_vector<ue_cell*, MAX_NOF_DU_CELLS> ue_cells;
};

/// UE handle used by the MAC scheduler to manage UE-specific state and operations.
class ue
{
public:
  ue(const ue_configuration&       cfg_,
     ue_logical_channel_repository dl_lch_repo,
     ue_drx_controller&            drx_ctrl,
     ue_ta_manager                 ta_mgr_,
     const ue_cell_lookup&         ue_cells);
  ue(const ue&)            = delete;
  ue(ue&&)                 = delete;
  ue& operator=(const ue&) = delete;
  ue& operator=(ue&&)      = delete;

  const du_ue_index_t ue_index;
  const rnti_t        crnti;

  void slot_indication(slot_point sl_tx);

  void deactivate();

  ue_cell*       find_cell(du_cell_index_t cell_index) { return cells.du_cells[cell_index]; }
  const ue_cell* find_cell(du_cell_index_t cell_index) const { return cells.du_cells[cell_index]; }

  /// \brief Fetch UE cell based on UE-specific cell identifier. E.g. PCell corresponds to ue_cell_index==0.
  ue_cell&       get_cell(ue_cell_index_t ue_cell_index) { return *cells.ue_cells[ue_cell_index]; }
  const ue_cell& get_cell(ue_cell_index_t ue_cell_index) const { return *cells.ue_cells[ue_cell_index]; }

  /// \brief Fetch UE PCell.
  ue_cell&       get_pcell() { return *cells.ue_cells[0]; }
  const ue_cell& get_pcell() const { return *cells.ue_cells[0]; }

  /// \brief Number of cells configured for the UE.
  unsigned nof_cells() const { return cells.ue_cells.size(); }

  /// \brief Returns dedicated configuration for the UE.
  const ue_configuration* ue_cfg_dedicated() const { return ue_ded_cfg; }

  bool is_ca_enabled() const { return cells.ue_cells.size() > 1; }

  void activate_cells(bounded_bitset<MAX_NOF_DU_CELLS> activ_bitmap) {}

  /// \brief Decides whether the UE sent a positive SR in a slot where the UE was scheduled a HARQ-ACK PUCCH F1 in a SR
  /// opportunity slot.
  ///
  /// The first time this function is called for a given slot, it saves the SR detection and SINR information of the
  /// received UCI indication. The second time it is called for the same slot, it compares the saved information with
  /// the new one to decide whether we got a positive SR or not.
  bool sr_and_f1_harq_ack_is_positive_sr(slot_point                                             uci_sl,
                                         const uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu& pucch_f0f1)
  {
    if (uci_sl == latest_sr_and_f1_harq_ack.sl_rx) {
      // Second UCI indication received for this slot. Decide if we got a positive SR or not.
      bool is_positive_sr = false;
      if (not latest_sr_and_f1_harq_ack.ul_sinr_dB.has_value()) {
        if (not pucch_f0f1.ul_sinr_dB.has_value()) {
          // If the SINR was invalid for both indications, assume negative SR.
          is_positive_sr = false;
        } else {
          // If only the first indication didn't have a valid SINR, assume the second one is correct.
          is_positive_sr = pucch_f0f1.sr_detected;
        }
      } else {
        if (pucch_f0f1.ul_sinr_dB.has_value()) {
          // If both indications have a valid SINR, compare them and assume the one with higher SINR is correct.
          if (*pucch_f0f1.ul_sinr_dB > *latest_sr_and_f1_harq_ack.ul_sinr_dB) {
            is_positive_sr = pucch_f0f1.sr_detected;
          } else {
            is_positive_sr = latest_sr_and_f1_harq_ack.sr_detected;
          }
        } else {
          // If this second indication doesn't have a valid SINR, assume the first one is correct.
          is_positive_sr = latest_sr_and_f1_harq_ack.sr_detected;
        }
      }

      // Reset slot of the last F1 UCI indication, to be ready for the next slot.
      latest_sr_and_f1_harq_ack.sl_rx = slot_point();
      return is_positive_sr;
    }

    // First F1 UCI indication for this slot received. Save its SINR and wait for the next one to decide.
    latest_sr_and_f1_harq_ack.sl_rx       = uci_sl;
    latest_sr_and_f1_harq_ack.sr_detected = pucch_f0f1.sr_detected;
    latest_sr_and_f1_harq_ack.ul_sinr_dB  = pucch_f0f1.ul_sinr_dB;
    return false;
  }

  /// \brief Handle received SR indication.
  void handle_sr_indication() { lc_ch_mgr.handle_sr_indication(); }

  /// \brief Handles received BSR indication by updating UE UL logical channel states.
  void handle_bsr_indication(const ul_bsr_indication_message& msg) { lc_ch_mgr.handle_bsr_indication(msg); }

  /// \brief Handles received N_TA update indication by forwarding it to Timing Advance manager.
  void handle_ul_n_ta_update_indication(du_cell_index_t cell_index, float ul_sinr, phy_time_unit n_ta_diff)
  {
    const ue_cell* ue_cc = find_cell(cell_index);
    ta_mgr.handle_ul_n_ta_update_indication(ue_cc->cfg().tag_id(), n_ta_diff.to_Tc(), ul_sinr);
  }

  /// \brief Handles MAC CE indication.
  void handle_dl_mac_ce_indication(const dl_mac_ce_indication& msg)
  {
    lc_ch_mgr.handle_mac_ce_indication({.ce_lcid = msg.ce_lcid, .ce_payload = dummy_ce_payload{}});
  }

  /// Called when a new UE configuration is passed to the scheduler, as part of the RRC Reconfiguration procedure.
  void handle_reconfiguration_request(const ue_configuration& new_cfg);

  /// \brief Handles DL Buffer State indication.
  void handle_dl_buffer_state_indication(lcid_t lcid, unsigned bs, slot_point hol_toa = {});

  /// \brief Computes the number of UL pending bytes that are not already allocated in a UL HARQ. The value is used
  /// to derive the required transport block size for an UL grant.
  unsigned pending_ul_newtx_bytes() const;

  /// \brief Retrieves UE DRX controller.
  ue_drx_controller& drx_controller() { return drx; }

  /// Retrieve UE logical channel manager.
  const ue_logical_channel_repository& logical_channels() const { return lc_ch_mgr; }
  ue_logical_channel_repository&       logical_channels() { return lc_ch_mgr; }

private:
  // Expert config parameters used for UE scheduler.
  const scheduler_ue_expert_config& expert_cfg;
  // Cell configuration. This is common to all UEs within the same cell.
  const cell_configuration& cell_cfg_common;
  // Dedicated configuration for the UE.
  const ue_configuration* ue_ded_cfg = nullptr;
  /// UE Logical Channel Manager.
  ue_logical_channel_repository lc_ch_mgr;
  /// UE Timing Advance Manager.
  ue_ta_manager ta_mgr;
  /// Controller of DRX active timer.
  ue_drx_controller& drx;
  /// Configured cells for the UE.
  const ue_cell_lookup& cells;

  slot_point last_sl_tx;

  /// Information about the last F1 UCI indication received in a slot where the UE was scheduled a HARQ-ACK PUCCH F1 in
  /// a SR opportunity slot.
  /// Used to compare the SINR of the HARQ-ACK PUCCH F1 and the SINR of the SR PUCCH F1, in order to decide whether the
  /// SR is valid or not.
  struct sr_and_f1_harq_ack_latest_uci_ind {
    slot_point           sl_rx;
    bool                 sr_detected;
    std::optional<float> ul_sinr_dB;
  };
  sr_and_f1_harq_ack_latest_uci_ind latest_sr_and_f1_harq_ack;
};

} // namespace ocudu
