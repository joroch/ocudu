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
#include "ocudu/ngap/ngap_context.h"
#include "ocudu/ngap/ngap_factory.h"
#include "ocudu/support/ocudu_assert.h"

using namespace ocudu;
using namespace ocucp;

xnap_repository::xnap_repository(xnap_repository_config cfg_) : cfg(cfg_), logger(cfg.logger)
{
  /// TODO
}

xnap_interface* xnap_repository::add_xnap(xnc_peer_index_t xnc_index, const cu_cp_configuration::ngap_config& config)
{
  // Create NGAP object
  auto it = xnap_db.insert(std::make_pair(xnc_index, xnap_context{}));
  ocudu_assert(it.second, "Unable to insert NGAP in map");

  /// TODO
  ///
  return nullptr;
}

std::map<xnc_peer_index_t, xnap_interface*> xnap_repository::get_xnaps()
{
  std::map<xnc_peer_index_t, xnap_interface*> ngaps;
  for (auto& peer : xnap_db) {
    ngaps.emplace(peer.first, peer.second.xnap.get());
  }
  return ngaps;
}
