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
#include "ocudu/xnap/xnap_message_notifier.h"

using namespace ocudu;
using namespace ocucp;

/// Context of a DU connection which is shared between the du_connection_manager and the f1ap_message_notifier.
class xnc_connection_manager::shared_xnc_connection_context
{
public:
  shared_xnc_connection_context(xnc_connection_manager& parent_) : parent(parent_) {}
  shared_xnc_connection_context(const shared_xnc_connection_context&)            = delete;
  shared_xnc_connection_context(shared_xnc_connection_context&&)                 = delete;
  shared_xnc_connection_context& operator=(const shared_xnc_connection_context&) = delete;
  shared_xnc_connection_context& operator=(shared_xnc_connection_context&&)      = delete;
  ~shared_xnc_connection_context() { disconnect(); }

  /// Assign a XNC repository index to the context. This is called when the SCTP assocation is created and
  /// attached to an XNAP handler.
  void connect_xnc(xnc_peer_index_t xnc_idx_)
  {
    xnc_idx     = xnc_idx_;
    msg_handler = parent.xnaps.find_xnap(xnc_idx);
  }

  /// Determines whether an XNAP message handlerrepository has been attached to this association.
  bool connected() const { return msg_handler != nullptr; }

  /// Deletes the associated DU repository, if it exists.
  void disconnect()
  {
    if (not connected()) {
      // DU was never allocated or was already removed.
      return;
    }

    // Notify DU that the connection is closed.
    // parent.handle_xnc_gw_connection_closed(du_idx);

    xnc_idx     = xnc_peer_index_t::invalid;
    msg_handler = nullptr;
  }

  /// Handle XNAP message coming from the SCTP GW.
  void handle_message(const xnap_message& msg)
  {
    if (not connected()) {
      parent.logger.warning("Discarding DU F1AP message. Cause: DU connection has been closed.");
    }

    // Forward message.
    msg_handler->handle_message(msg);
  }

private:
  xnc_connection_manager& parent;
  xnc_peer_index_t        xnc_idx     = xnc_peer_index_t::invalid;
  xnap_message_handler*   msg_handler = nullptr;
};

/// Notifier used to forward Rx F1AP messages from the F1-C GW to CU-CP in a thread safe manner.
class xnc_connection_manager::xnc_gw_to_cu_cp_pdu_adapter final : public xnap_message_notifier
{
public:
  xnc_gw_to_cu_cp_pdu_adapter(xnc_connection_manager& parent_, std::shared_ptr<shared_xnc_connection_context> ctxt_) :
    parent(parent_), ctxt(std::move(ctxt_))
  {
  }

  ~xnc_gw_to_cu_cp_pdu_adapter() override
  {
    // Defer destruction of context to CU-CP execution context.
    // Note: We make a copy of the shared_ptr of the context to extend its lifetime to when the defer callback actually
    // gets executed.
    // Note: We don't use move because the defer may fail.
    while (not parent.cu_cp_exec.defer([ctxt_cpy = ctxt]() { ctxt_cpy->disconnect(); })) {
      parent.logger.error("Failed to schedule DU removal task. Retrying...");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void on_new_message(const xnap_message& msg) override
  {
    // Dispatch the F1AP Rx message handling to the CU-CP executor.
    while (not parent.cu_cp_exec.execute([this, msg]() { ctxt->handle_message(msg); })) {
      parent.logger.error("Failed to dispatch F1AP message to CU-CP. Retrying...");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

private:
  xnc_connection_manager&                        parent;
  std::shared_ptr<shared_xnc_connection_context> ctxt;
};

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

void xnc_connection_manager::start()
{
  fmt::println("XNC connection manager start");
  // Schedules setup routine to be executed in sequence with other CU-CP procedures.
  common_task_sched.schedule_async_task(launch_async(
      [this, xn_it = std::map<xnc_peer_index_t, xnap_interface*>::iterator{}, xnaps_map = xnaps.get_xnaps()](
          coro_context<async_task<void>>& ctx) mutable {
        CORO_BEGIN(ctx);

        // TODO try to connect to all neighbours.
        for (xn_it = xnaps_map.begin(); xn_it != xnaps_map.end(); ++xn_it) {
          CORO_AWAIT(xn_it->second->handle_xn_setup_request_required(1));
        }

        CORO_RETURN();
      }));
}

void xnc_connection_manager::connect_to_neighbours()
{
  std::map<xnc_peer_index_t, xnap_interface*> xn = xnaps.get_xnaps();
  for (const std::pair<const xnc_peer_index_t, xnap_interface*>& xnap_it : xn) {
    xnap_it.second->handle_xn_setup_request_required(1);
  }
}

void xnc_connection_manager::stop()
{
  if (stopped) {
    return;
  }

  /*
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
  */

  stopped = true;
}

std::unique_ptr<xnap_message_notifier>
xnc_connection_manager::handle_new_xnc_connection(std::unique_ptr<xnap_message_notifier> xnap_tx_pdu_notifier,
                                                  const sctp_association_info&           assoc_info)
{
  // Note: This function may be called from a different execution context than the CU-CP.
  fmt::println("Handling new XN-C");
  if (stopped.load(std::memory_order_acquire)) {
    // CU-CP is in the process of being stopped.
    return nullptr;
  }

  // We create a "detached" notifier, that has no associated XNAP yet.
  auto shared_ctxt     = std::make_shared<shared_xnc_connection_context>(*this);
  auto rx_pdu_notifier = std::make_unique<xnc_gw_to_cu_cp_pdu_adapter>(*this, shared_ctxt);

  // Find XNAP neighbour. This needs to be done over the CU-CP execution context, so
  // we dispatch the task to find the correct XNAP and "attach" it to the notifier
  while (not cu_cp_exec.execute(
      [this, shared_ctxt, sender_notifier = std::move(xnap_tx_pdu_notifier), addr = assoc_info.peer_addr]() mutable {
        // Find XNAP based on address of peer.
        // TODO.
        xnc_peer_index_t xnc_index = xnaps.find_xnap(addr);
        if (xnc_index == xnc_peer_index_t::invalid) {
          logger.warning("Rejecting new DU connection. Cause: Failed to create a new DU");
          return;
        }

        // Register the XNAP peer in the shared XNAP connection context.
        shared_ctxt->connect_xnc(xnc_index);

        if (not xnc_connections.insert(std::make_pair(xnc_index, std::move(shared_ctxt))).second) {
          logger.error("Failed to store new DU connection {}", fmt::underlying(xnc_index));
          return;
        }

        // logger.info("Added TNL association to XN-C {}", xnc_index);
      })) {
    logger.debug("Failed to dispatch CU-CP DU connection task. Retrying...");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return rx_pdu_notifier;
}
