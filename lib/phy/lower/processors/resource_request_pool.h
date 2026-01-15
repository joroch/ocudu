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

#include "ocudu/adt/circular_array.h"
#include "ocudu/ran/slot_point.h"
#include <mutex>
#include <utility>

namespace ocudu {

/// \brief Thread-safe resource request pool.
///
/// Contains a pool of requests. A request consist of a slot point and a templated resource. Requests are indexed by
/// slots. The pool access is thread-safe and it is done by exchange requests.
///
/// \tparam ResourceType Resource type.
template <typename ResourceType>
class resource_request_pool
{
public:
  /// Maximum number of requests contained in the array.
  static constexpr unsigned request_array_size = 16;

  /// Internal storage type.
  struct request_type {
    slot_point   slot;
    ResourceType resource;
  };

  /// \brief Exchanges a request from the pool by the given one.
  /// \param[in] request The given request, it is copied into <tt>request.slot.system_slot() % request_array_size</tt>.
  /// \return The previous request at position <tt>request.slot.system_slot() % request_array_size</tt>.
  request_type exchange(request_type request)
  {
    return requests[request.slot.system_slot()].exchange(std::move(request));
  }

private:
  /// Wraps the request in a thread-safe access.
  class request_wrapper
  {
  public:
    /// Default constructor - constructs an empty request.
    request_wrapper() : request({slot_point(), ResourceType()}) {}

    /// \brief Exchanges the previous request with a new request.
    /// \param[in] new_request New request.
    /// \return A copy of the previous request if there is no other exchange happening simultaneously, otherwise the
    /// same request.
    request_type exchange(request_type new_request)
    {
      uint16_t slot_key = new_request.slot.system_slot() % request_array_size;

      // Return the same request if there's another simultaneous exchange for this slot.
      if (OCUDU_UNLIKELY((reserved_flag.load() >> slot_key) & 1)) {
        return std::move(new_request);
      }

      // Flag this exchange to avoid concurrent access for this slot.
      reserved_flag.fetch_or(static_cast<uint16_t>(1u << slot_key));

      request_type old_request = std::move(request);
      request                  = std::move(new_request);

      // Unflag the exchange.
      reserved_flag.fetch_and(static_cast<uint16_t>(~(1u << slot_key)));

      return old_request;
    }

  private:
    request_type          request;
    std::atomic<uint16_t> reserved_flag = 0;
  };

  /// Request storage, indexed by slots.
  circular_array<request_wrapper, request_array_size> requests;
};

} // namespace ocudu
