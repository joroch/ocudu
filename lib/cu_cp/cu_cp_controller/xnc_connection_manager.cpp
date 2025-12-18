/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "xnc_connection_manager.h"
#include "../cu_cp_impl_interface.h"
#include "../routines/amf_connection_removal_routine.h"
#include "../routines/amf_connection_setup_routine.h"
#include "../routines/amf_reconnection_routine.h"
#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/ngap/ngap.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/support/synchronization/baton.h"
#include <chrono>
#include <thread>

using namespace ocudu;
using namespace ocucp;

xnc_connection_manager::xnc_connection_manager(xnap_repository&       xnaps_,
                                               timer_manager&         timers_,
                                               task_executor&         cu_cp_exec_,
                                               common_task_scheduler& common_task_sched_) :
  xnaps(xnaps_),
  timers(timers_),
  cu_cp_exec(cu_cp_exec_),
  common_task_sched(common_task_sched_),
  logger(ocudulog::fetch_basic_logger("CU-CP"))
{
}

void xnc_connection_manager::connect_to_neighbours()
{
  // Schedules setup routine to be executed in sequence with other CU-CP procedures.
  common_task_sched.schedule_async_task(launch_async([this](coro_context<async_task<void>>& ctx) mutable {
    CORO_BEGIN(ctx);

    // TODO try to connect to all neighbours.
    // CORO_AWAIT_VALUE(success, start_amf_connection_setup(ngaps, amfs_connected));

    CORO_RETURN();
  }));
}

void xnc_connection_manager::stop()
{
  if (stopped) {
    return;
  }

  baton               stop_baton;
  scoped_baton_sender signal_stop{stop_baton};

  // Stop and delete AMF connections.
  while (not cu_cp_exec.defer([this, signal_stop = std::move(signal_stop)]() mutable {
    common_task_sched.schedule_async_task(
        launch_async([this, signal_stop = std::move(signal_stop)](coro_context<async_task<void>>& ctx) mutable {
          CORO_BEGIN(ctx);
          // Disconnect XNAP connection(s).
          CORO_AWAIT(disconnect_neighbours());

          // XNAP disconnection successfully finished.
          // Dispatch main async task loop destruction via defer so that the current coroutine ends successfully.
          while (not cu_cp_exec.defer([signal_stop = std::move(signal_stop)]() mutable { signal_stop.post(); })) {
            logger.warning("Unable to stop AMF Manager. Retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }

          CORO_RETURN();
        }));
  })) {
    logger.warning("Failed to dispatch AMF stop task. Retrying...");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Wait for AMF stop to complete.
  stop_baton.wait();

  stopped = true;
}

std::unique_ptr<xnap_message_notifier>
xnc_connection_manager::handle_new_xnc_connection(std::unique_ptr<xnap_message_notifier> f1ap_tx_pdu_notifier)
{
  // Note: This function may be called from a different execution context than the CU-CP.

  if (stopped.load(std::memory_order_acquire)) {
    // CU-CP is in the process of being stopped.
    return nullptr;
  }

  // Verify that there is space for new DU connection.
  // TODO.

  // We create a "detached" notifier, that has no associated DU processor yet.
  auto shared_ctxt     = std::make_shared<shared_du_connection_context>(*this);
  auto rx_pdu_notifier = std::make_unique<f1_gw_to_cu_cp_pdu_adapter>(*this, shared_ctxt);

  // We dispatch the task to allocate a DU processor and "attach" it to the notifier
  while (not cu_cp_exec.execute([this, shared_ctxt, sender_notifier = std::move(f1ap_tx_pdu_notifier)]() mutable {
    // Create a new DU processor.
    du_index_t du_index = dus.add_du(std::move(sender_notifier));
    if (du_index == du_index_t::invalid) {
      logger.warning("Rejecting new DU connection. Cause: Failed to create a new DU");
      return;
    }

    // Register the allocated DU processor index in the DU connection context.
    shared_ctxt->connect_du(du_index);

    if (not du_connections.insert(std::make_pair(du_index, std::move(shared_ctxt))).second) {
      logger.error("Failed to store new DU connection {}", du_index);
      return;
    }

    logger.info("Added TNL connection to DU {}", du_index);
  })) {
    logger.debug("Failed to dispatch CU-CP DU connection task. Retrying...");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return rx_pdu_notifier;
}
