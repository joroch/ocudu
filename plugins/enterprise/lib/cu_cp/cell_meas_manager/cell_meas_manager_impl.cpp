/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cell_meas_manager/cell_meas_manager_impl.h"
#include "ue_manager/ue_manager_impl.h"
#include "ocudu/adt/expected.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/ran/pci.h"

using namespace ocudu;
using namespace ocucp;

static expected<nr_cell_identity, std::string> find_nci(const cell_meas_manager_cfg& meas_mng_cfg, pci_t pci)
{
  for (const auto& [nci, cell_cfg] : meas_mng_cfg.cells) {
    if (cell_cfg.serving_cell_cfg.pci == pci) {
      return nci;
    }
  }
  return make_unexpected(fmt::format("No NCI found for PCI={}", pci));
}

static expected<cell_measurement_positioning_info, std::string> generate_measurement_objects_for_positioning(
    ue_index_t                                                  ue_index,
    ue_manager&                                                 ue_mng,
    const cell_meas_manager_cfg&                                cfg,
    const std::map<nr_cell_identity, serving_cell_meas_config>& nci_to_serving_cell_meas_config,
    const rrc_meas_results&                                     meas_results,
    ocudulog::basic_logger&                                     logger)
{
  cu_cp_ue* ue = ue_mng.find_ue(ue_index);
  if (ue == nullptr) {
    return make_unexpected(fmt::format("UE not found", ue_index));
  }

  cell_meas_manager_ue_context& ue_meas_context = ue_mng.get_measurement_context(ue_index);
  ocudu_assert(ue_meas_context.meas_id_to_meas_context.find(meas_results.meas_id) !=
                   ue_meas_context.meas_id_to_meas_context.end(),
               "ue={}: Measurement result for unknown meas_id={} received",
               fmt::underlying(ue_index),
               fmt::underlying(meas_results.meas_id));

  meas_context_t& meas_ctxt = ue_meas_context.meas_id_to_meas_context.at(meas_results.meas_id);

  // Prepare measurement to forward measurement to CU-CP.
  cell_measurement_positioning_info pos_info;
  // Prepare serving cell measurement.
  {
    pci_t                    serving_cell_pci = ue->get_pci();
    nr_cell_identity         serving_nci;
    serving_cell_meas_config serv_cell_cfg;
    if (meas_ctxt.pci == serving_cell_pci) {
      // We received a measurement for the serving cell.
      serving_nci = meas_ctxt.nci;

    } else {
      // Find the serving cell.
      auto nci = find_nci(cfg, serving_cell_pci);
      if (!nci.has_value()) {
        return make_unexpected(fmt::format("{}", nci.error()));
      }
      serving_nci = nci.value();
    }

    if (nci_to_serving_cell_meas_config.find(meas_ctxt.nci) == nci_to_serving_cell_meas_config.end()) {
      return make_unexpected(fmt::format("No serving cell config found for nci={:#x}", meas_ctxt.nci));
    }
    serv_cell_cfg            = nci_to_serving_cell_meas_config.at(meas_ctxt.nci);
    pos_info.serving_cell_id = nr_cell_global_id_t{serv_cell_cfg.plmn, serv_cell_cfg.nci};
    pos_info.cell_measurements.emplace(serv_cell_cfg.nci,
                                       cell_measurement_positioning_info::cell_measurement_item_t{
                                           nr_cell_global_id_t{serv_cell_cfg.plmn, serv_cell_cfg.nci},
                                           serv_cell_cfg.ssb_arfcn.value(),
                                           meas_results.meas_result_serving_mo_list.begin()->meas_result_serving_cell});

    // TODO: Configure rs_idx_results and replace hardcoded values.
    rrc_meas_result_nr& meas_res = pos_info.cell_measurements.at(serv_cell_cfg.nci).meas_result;

    rrc_meas_result_nr::rs_idx_results_ rs_idx_results;
    if (meas_res.cell_results.results_ssb_cell.has_value()) {
      rs_idx_results.results_ssb_idxes.emplace(1, rrc_results_per_ssb_idx{1, meas_res.cell_results.results_ssb_cell});
    }
    if (meas_res.cell_results.results_csi_rs_cell.has_value()) {
      rs_idx_results.results_csi_rs_idxes.emplace(
          1, rrc_results_per_csi_rs_idx{1, meas_res.cell_results.results_csi_rs_cell});
    }
    meas_res.rs_idx_results = rs_idx_results;

    // TODO: Handle multiple serving cells.
  }
  // Prepare neighbor cell measurement.
  {
    if (meas_results.meas_result_neigh_cells.has_value()) {
      for (const rrc_meas_result_nr& neigh_meas_result : meas_results.meas_result_neigh_cells->meas_result_list_nr) {
        if (!neigh_meas_result.pci.has_value()) {
          logger.debug("ue={}: Neighbor cell measurement without PCI received", ue_index);
          continue;
        }

        // Find the serving cell config of the neighbor cell.
        expected<nr_cell_identity, std::string> nci = find_nci(cfg, neigh_meas_result.pci.value());
        if (!nci.has_value()) {
          return make_unexpected(fmt::format("{}", nci.error()));
        }

        if (nci_to_serving_cell_meas_config.find(nci.value()) == nci_to_serving_cell_meas_config.end()) {
          return make_unexpected(fmt::format("No serving cell config found for nci={:#x}", nci.value()));
        }
        serving_cell_meas_config ncell_meas_cfg = nci_to_serving_cell_meas_config.at(nci.value());
        pos_info.cell_measurements.emplace(nci.value(),
                                           cell_measurement_positioning_info::cell_measurement_item_t{
                                               nr_cell_global_id_t{ncell_meas_cfg.plmn, nci.value()},
                                               ncell_meas_cfg.ssb_arfcn.value(),
                                               neigh_meas_result});

        // TODO: Configure rs_idx_results and replace hardcoded values.
        rrc_meas_result_nr& meas_res = pos_info.cell_measurements.at(nci.value()).meas_result;

        rrc_meas_result_nr::rs_idx_results_ rs_idx_results;
        if (meas_res.cell_results.results_ssb_cell.has_value()) {
          rs_idx_results.results_ssb_idxes.emplace(1,
                                                   rrc_results_per_ssb_idx{1, meas_res.cell_results.results_ssb_cell});
        }
        if (meas_res.cell_results.results_csi_rs_cell.has_value()) {
          rs_idx_results.results_csi_rs_idxes.emplace(
              1, rrc_results_per_csi_rs_idx{1, meas_res.cell_results.results_csi_rs_cell});
        }
        meas_res.rs_idx_results = rs_idx_results;
      }
    }
  }
  return pos_info;
}

void cell_meas_manager::store_measurement_results(ue_index_t ue_index, const rrc_meas_results& meas_results)
{
  if (ue_mng.find_ue(ue_index) == nullptr) {
    logger.info("ue={}: Not storing positioning measurement. Cause: UE not found", ue_index);
  }

  cell_meas_manager_ue_context& ue_meas_context = ue_mng.get_measurement_context(ue_index);

  expected<cell_measurement_positioning_info, std::string> pos_info = generate_measurement_objects_for_positioning(
      ue_index, ue_mng, cfg, nci_to_serving_cell_meas_config, meas_results, logger);

  if (!pos_info.has_value()) {
    logger.info("ue={}: Not storing positioning measurement. Cause: {}", ue_index, pos_info.error());
    return;
  }

  // Store measurement results in measurement context.
  logger.info("ue={}: Storing positioning measurement", ue_index);
  ue_meas_context.meas_results = pos_info.value();
}
