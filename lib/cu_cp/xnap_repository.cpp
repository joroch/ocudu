/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "xnap_repository.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/ocudu_assert.h"
#include "ocudu/xnap/xnap_factory.h"

using namespace ocudu;
using namespace ocucp;

xnap_repository::xnap_repository(xnap_repository_config cfg_) : cfg(cfg_), logger(cfg.logger)
{
  /// TODO
  for (const cu_cp_configuration::xnap_config& xn : cfg.cu_cp.xnap.xnaps) {
    uint16_t idx = 0;
    add_xnap(uint_to_xnc_peer_index(idx++), xn);
  }
}

std::map<xnc_peer_index_t, xnap_interface*> xnap_repository::get_xnaps()
{
  std::map<xnc_peer_index_t, xnap_interface*> ngaps;
  for (auto& peer : xnap_db) {
    ngaps.emplace(peer.first, peer.second.xnap.get());
  }
  return ngaps;
}

xnap_interface* xnap_repository::add_xnap(xnc_peer_index_t xnc_index, const cu_cp_configuration::xnap_config& config)
{
  // Create XNAP object
  xnap_context xnap_ctxt;
  xnap_ctxt.peer_addr = config.peer_addr;
  // TODO connect XNAP handler to CU-CP.

  xnap_configuration xnap_cfg = {};
  xnap_cfg.peer_addr          = config.peer_addr;
  xnap_ctxt.xnap              = create_xnap(xnap_cfg,
                               *config.xnc_gw,
                               xnap_ctxt.xnap_to_cu_cp_notifier,
                               *cfg.cu_cp.services.timers,
                               *cfg.cu_cp.services.cu_cp_executor);

  auto it = xnap_db.insert(std::make_pair(xnc_index, std::move(xnap_ctxt)));
  ocudu_assert(it.second, "Unable to insert NGAP in map");

  /// TODO
  ///
  return nullptr;
}

xnap_interface* xnap_repository::find_xnap(xnc_peer_index_t xnc_index)
{
  auto it = xnap_db.find(xnc_index);
  if (it == xnap_db.end()) {
    return nullptr;
  }
  return it->second.xnap.get();
}

xnc_peer_index_t xnap_repository::find_xnap(const transport_layer_address& peer_addr)
{
  for (const std::pair<const xnc_peer_index_t, xnap_context>& xn : xnap_db) {
    auto peer = xn.second.xnap->get_peer_address();
    if (peer == peer_addr) {
      fmt::println("Found XN-C: {}", fmt::underlying(xn.first));
      return xn.first;
    }
  }
  fmt::println("Could not find XN-C");
  return xnc_peer_index_t::invalid;
}
