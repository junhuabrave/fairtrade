#include "risk_engine.h"
#include <cassert>

namespace trading::risk {

AccountRiskState& RiskEngine::get_or_create_account(uint64_t account_id) {
    std::lock_guard<std::mutex> g(state_mutex_);
    auto it = accounts_.find(account_id);
    if (it == accounts_.end()) {
        auto em = accounts_.try_emplace(account_id);
        return em.first->second;
    }
    return it->second;
}

const AccountRiskState* RiskEngine::get_account(uint64_t account_id) const noexcept {
    std::lock_guard<std::mutex> g(state_mutex_);
    auto it = accounts_.find(account_id);
    if (it == accounts_.end()) return nullptr;
    return &it->second;
}

int32_t RiskEngine::validate_order_new(uint64_t account_id, const sbe::OrderNewMessage& order) noexcept {
    if (global_lock_.load(std::memory_order_acquire)) return RISK_REJECT_ACCOUNT_LOCKED;
    auto& acct = get_or_create_account(account_id);
    if (acct.locked.load(std::memory_order_acquire)) return RISK_REJECT_ACCOUNT_LOCKED;

    int64_t order_value = calculate_order_value(order.price, order.quantity);
    if (order_value > acct.max_single_order_value.load(std::memory_order_acquire)) {
        return RISK_REJECT_MAX_ORDER_VALUE;
    }

    int64_t projected_pnl = acct.daily_pnl.load(std::memory_order_relaxed) - order_value; // conservative
    if (projected_pnl < acct.daily_loss_limit.load(std::memory_order_acquire)) {
        return RISK_REJECT_DAILY_LOSS_LIMIT;
    }

    acct.order_count.fetch_add(1, std::memory_order_relaxed);
    return RISK_CHECK_PASSED;
}

int32_t RiskEngine::validate_order_cancel(uint64_t account_id, const sbe::OrderCancelMessage& /*cancel*/) noexcept {
    if (global_lock_.load(std::memory_order_acquire)) return RISK_REJECT_ACCOUNT_LOCKED;
    auto const* acct = get_account(account_id);
    if (!acct) return RISK_CHECK_PASSED;
    if (acct->locked.load(std::memory_order_acquire)) return RISK_REJECT_ACCOUNT_LOCKED;
    return RISK_CHECK_PASSED;
}

void RiskEngine::update_position(uint64_t account_id, int64_t exposure_delta, int64_t pnl_delta) noexcept {
    auto& acct = get_or_create_account(account_id);
    acct.total_exposure.fetch_add(exposure_delta, std::memory_order_relaxed);
    acct.daily_pnl.fetch_add(pnl_delta, std::memory_order_relaxed);
}

void RiskEngine::record_execution(uint64_t account_id, int64_t /*value*/) noexcept {
    // placeholder hook for metrics; kept minimal for low-latency
    (void)account_id;
}

bool RiskEngine::is_account_locked(uint64_t account_id) const noexcept {
    auto const* acct = get_account(account_id);
    return acct && acct->locked.load(std::memory_order_acquire);
}

int64_t RiskEngine::get_daily_pnl(uint64_t account_id) const noexcept {
    auto const* acct = get_account(account_id);
    if (!acct) return 0;
    return acct->daily_pnl.load(std::memory_order_acquire);
}

int64_t RiskEngine::get_total_exposure(uint64_t account_id) const noexcept {
    auto const* acct = get_account(account_id);
    if (!acct) return 0;
    return acct->total_exposure.load(std::memory_order_acquire);
}

bool RiskEngine::administrative_signoff(uint64_t signoff_key) noexcept {
    if (signoff_key != SECURE_SIGNOFF_2026) return false;
    global_lock_.store(true, std::memory_order_release);
    // lock all known accounts
    std::lock_guard<std::mutex> g(state_mutex_);
    for (auto &p : accounts_) p.second.locked.store(true, std::memory_order_release);
    return true;
}

void RiskEngine::set_account_limits(uint64_t account_id, int64_t max_order_value, int64_t loss_limit) {
    auto& acct = get_or_create_account(account_id);
    acct.max_single_order_value.store(max_order_value, std::memory_order_release);
    acct.daily_loss_limit.store(loss_limit, std::memory_order_release);
}

} // namespace trading::risk
