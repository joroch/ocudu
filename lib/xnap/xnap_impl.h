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

#include "procedures/xnap_transaction_manager.h"
#include "ocudu/support/executors/task_executor.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"
#include "ocudu/xnap/xnap.h"
#include <memory>

namespace ocudu::ocucp {

class xnap_impl final : public xnap_interface
{
public:
  xnap_impl(const xnap_configuration& xnap_cfg_,
            xnc_connection_gateway&   xnc_gw_,
            xnap_cu_cp_notifier&      cu_cp_notifier_,
            timer_manager&            timers_,
            task_executor&            ctrl_exec_);
  ~xnap_impl();

  // XNAP connection manager functions.
  bool                    handle_xn_peer_tnl_connection_request() override;
  async_task<void>        handle_xn_setup_request_required(unsigned max_setup_retries) override;
  transport_layer_address get_peer_address() override { return peer_addr; }
  void set_tx_association(xnap_message_notifier* tx_notifier_) override { tx_notifier = tx_notifier_; }

  // XNAP message handling.
  void handle_message(const xnap_message& msg) override;

private:
  class tx_pdu_notifier_with_logging final : public xnap_message_notifier
  {
  public:
    tx_pdu_notifier_with_logging(xnap_impl& parent_) : parent(parent_) {}

    ~tx_pdu_notifier_with_logging()
    {
      if (decorated) {
        decorated.reset();
      }
    }

    void connect(std::unique_ptr<xnap_message_notifier> decorated_) { decorated = std::move(decorated_); }

    void disconnect()
    {
      if (is_connected()) {
        decorated.reset();
      }
    }

    bool is_connected() const { return decorated != nullptr; }

    void on_new_message(const xnap_message& msg) override;

  private:
    xnap_impl&                             parent;
    std::unique_ptr<xnap_message_notifier> decorated;
  };

  /// Message handling.
  void handle_initiating_message(const asn1::xnap::init_msg_s& msg);
  void handle_successful_outcome(const asn1::xnap::successful_outcome_s& msg);
  void handle_unsuccessful_outcome(const asn1::xnap::unsuccessful_outcome_s& msg);

  void handle_xn_setup_request(const asn1::xnap::xn_setup_request_s& msg);

  /// \brief Log NGAP RX PDU.
  void log_rx_pdu(const xnap_message& msg);

  ocudulog::basic_logger& logger;
  xnap_configuration      xnap_cfg;
  transport_layer_address peer_addr;

  xnc_connection_gateway& xnc_gw;
  xnap_cu_cp_notifier&    cu_cp_notifier;
  timer_manager&          timers;
  task_executor&          ctrl_exec;

  xnap_transaction_manager transaction_mng;

  tx_pdu_notifier_with_logging tx_pdu_notifier;
  xnap_message_notifier*       tx_notifier = nullptr;
};

} // namespace ocudu::ocucp
