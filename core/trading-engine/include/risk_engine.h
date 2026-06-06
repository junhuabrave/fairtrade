#pragma once

#include <cstdint>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "sbe_messages.h"

namespace trading::risk {

static constexpr int32_t RISK_CHECK_PASSED = 0;
static constexpr int32_t RISK_REJECT_MAX_ORDER_VALUE = -1;
static constexpr int32_t RISK_REJECT_DAILY_LOSS_LIMIT = -2;
static constexpr int32_t RISK_REJECT_ACCOUNT_LOCKED = -99;

struct AccountRiskState {
    alignas(64) std::atomic<int64_t> daily_pnl{0};
    alignas(64) std::atomic<int64_t> total_exposure{0};
    alignas(64) std::atomic<bool> locked{false};
    alignas(64) std::atomic<uint64_t> order_count{0};
    alignas(64) std::atomic<int64_t> max_single_order_value{1000000000};
    alignas(64) std::atomic<int64_t> daily_loss_limit{-500000000};
    
    void reset_daily() noexcept {
        daily_pnl.store(0, std::memory_order_relaxed);
        total_exposure.store(0, std::memory_order_relaxed);
        order_count.store(0, std::memory_order_relaxed);
        locked.store(false, std::memory_order_release);
    }
};

class RiskEngine {
public:
    static constexpr uint64_t SECURE_SIGNOFF_2026 = 0x2026DEADBEEFCAFEULL;
    
    RiskEngine() = default;
    ~RiskEngine() = default;
    
    RiskEngine(const RiskEngine&) = delete;
    RiskEngine& operator=(const RiskEngine&) = delete;
    RiskEngine(RiskEngine&&) = delete;
    RiskEngine& operator=(RiskEngine&&) = delete;
    
    int32_t validate_order_new(uint64_t account_id, const sbe::OrderNewMessage& order) noexcept;
    int32_t validate_order_cancel(uint64_t account_id, const sbe::OrderCancelMessage& cancel) noexcept;
    
    void update_position(uint64_t account_id, int64_t exposure_delta, int64_t pnl_delta) noexcept;
    void record_execution(uint64_t account_id, int64_t value) noexcept;
    
    bool is_account_locked(uint64_t account_id) const noexcept;
    int64_t get_daily_pnl(uint64_t account_id) const noexcept;
    int64_t get_total_exposure(uint64_t account_id) const noexcept;
    
    bool administrative_signoff(uint64_t signoff_key) noexcept;
    void set_account_limits(uint64_t account_id, int64_t max_order_value, int64_t loss_limit);
    
private:
    mutable std::mutex state_mutex_;
    std::unordered_map<uint64_t, AccountRiskState> accounts_;
    alignas(64) std::atomic<bool> global_lock_{false};
    
    AccountRiskState& get_or_create_account(uint64_t account_id);
    const AccountRiskState* get_account(uint64_t account_id) const noexcept;
};

inline int64_t calculate_order_value(int64_t price, uint32_t quantity) noexcept {
    return price * static_cast<int64_t>(quantity);
}

} // namespace trading::risk
