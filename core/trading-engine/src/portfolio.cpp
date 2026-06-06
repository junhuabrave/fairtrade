#include "portfolio.h"
#include "ring_logger.h"
#include <cassert>

namespace trading::portfolio {

void Position::update_on_fill(int64_t fill_qty, int64_t fill_price, uint64_t timestamp_ns) noexcept {
    int64_t prev_qty = quantity.load(std::memory_order_relaxed);
    int64_t prev_total_cost = total_cost.load(std::memory_order_relaxed);

    int64_t new_total_cost = prev_total_cost + fill_qty * fill_price;
    int64_t new_qty = prev_qty + fill_qty;

    total_cost.store(new_total_cost, std::memory_order_relaxed);
    quantity.store(new_qty, std::memory_order_relaxed);
    if (new_qty != 0) {
        avg_cost_basis.store(calculate_vwap(new_total_cost, new_qty), std::memory_order_relaxed);
    }
    trade_count.fetch_add(1, std::memory_order_relaxed);
    last_update_ns.store(timestamp_ns, std::memory_order_release);
}

int64_t Position::get_market_value(int64_t current_price) const noexcept {
    return quantity.load(std::memory_order_acquire) * current_price;
}

void Position::mark_to_market(int64_t current_price) noexcept {
    int64_t mv = get_market_value(current_price);
    unrealized_pnl.store(mv - total_cost.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void AccountPortfolio::apply_fill(uint64_t symbol_id, int64_t qty, int64_t price, uint64_t timestamp_ns) {
    auto &pos = positions[symbol_id];
    pos.update_on_fill(qty, price, timestamp_ns);
}

int64_t AccountPortfolio::get_total_position_value(uint64_t symbol_id) const noexcept {
    auto it = positions.find(symbol_id);
    if (it == positions.end()) return 0;
    return it->second.get_market_value( it->second.avg_cost_basis.load(std::memory_order_relaxed) );
}

void AccountPortfolio::recalculate_total_value(const std::unordered_map<uint64_t, int64_t>& market_prices) {
    int64_t total = 0;
    for (auto &p : positions) {
        auto it = market_prices.find(p.first);
        int64_t price = (it != market_prices.end()) ? it->second : p.second.avg_cost_basis.load(std::memory_order_relaxed);
        total += p.second.get_market_value(price);
    }
    total_value.store(total, std::memory_order_release);
}

AccountPortfolio& PortfolioManager::get_or_create_portfolio(uint64_t account_id) {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    auto it = portfolios_.find(account_id);
    if (it == portfolios_.end()) {
        auto em = portfolios_.try_emplace(account_id);
        em.first->second.account_id = account_id;
        return em.first->second;
    }
    return it->second;
}

void PortfolioManager::process_execution(uint64_t account_id, uint64_t symbol_id, int64_t qty,
                                        int64_t price, uint64_t timestamp_ns) {
    auto &acct = get_or_create_portfolio(account_id);
    acct.apply_fill(symbol_id, qty, price, timestamp_ns);
}

PositionSnapshot PortfolioManager::get_position(uint64_t account_id, uint64_t symbol_id) const {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    auto it = portfolios_.find(account_id);
    if (it == portfolios_.end()) return PositionSnapshot();
    auto pit = it->second.positions.find(symbol_id);
    if (pit == it->second.positions.end()) return PositionSnapshot();
    // Build a plain-data snapshot from atomics for safe return
    PositionSnapshot snap;
    snap.quantity = pit->second.quantity.load(std::memory_order_acquire);
    snap.avg_cost_basis = pit->second.avg_cost_basis.load(std::memory_order_relaxed);
    snap.total_cost = pit->second.total_cost.load(std::memory_order_relaxed);
    snap.realized_pnl = pit->second.realized_pnl.load(std::memory_order_relaxed);
    snap.unrealized_pnl = pit->second.unrealized_pnl.load(std::memory_order_relaxed);
    snap.trade_count = pit->second.trade_count.load(std::memory_order_relaxed);
    snap.last_update_ns = pit->second.last_update_ns.load(std::memory_order_acquire);
    return snap;
}

AccountPortfolioSnapshot PortfolioManager::get_account_summary(uint64_t account_id) const {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    auto it = portfolios_.find(account_id);
    if (it == portfolios_.end()) return AccountPortfolioSnapshot();
    AccountPortfolioSnapshot snap;
    snap.account_id = it->second.account_id;
    snap.total_value = it->second.total_value.load(std::memory_order_acquire);
    snap.buying_power = it->second.buying_power.load(std::memory_order_acquire);
    snap.locked = it->second.locked.load(std::memory_order_acquire);
    snap.last_signoff_ns = it->second.last_signoff_ns.load(std::memory_order_acquire);
    for (auto &p : it->second.positions) {
        PositionSnapshot ps;
        ps.quantity = p.second.quantity.load(std::memory_order_acquire);
        ps.avg_cost_basis = p.second.avg_cost_basis.load(std::memory_order_relaxed);
        ps.total_cost = p.second.total_cost.load(std::memory_order_relaxed);
        ps.realized_pnl = p.second.realized_pnl.load(std::memory_order_relaxed);
        ps.unrealized_pnl = p.second.unrealized_pnl.load(std::memory_order_relaxed);
        ps.trade_count = p.second.trade_count.load(std::memory_order_relaxed);
        ps.last_update_ns = p.second.last_update_ns.load(std::memory_order_acquire);
        snap.positions.emplace(p.first, std::move(ps));
    }
    return snap;
}

std::vector<uint64_t> PortfolioManager::get_all_accounts() const {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    std::vector<uint64_t> out;
    out.reserve(portfolios_.size());
    for (auto &p : portfolios_) out.push_back(p.first);
    return out;
}

bool PortfolioManager::signoff_and_lock(uint64_t account_id, uint64_t signoff_key) noexcept {
    if (signoff_key != SECURE_SIGNOFF_2026) return false;
    auto &acct = get_or_create_portfolio(account_id);
    acct.locked.store(true, std::memory_order_release);
    acct.last_signoff_ns.store(::trading::log::get_timestamp_ns(), std::memory_order_release);
    return true;
}

bool PortfolioManager::is_account_locked(uint64_t account_id) const noexcept {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    auto it = portfolios_.find(account_id);
    if (it == portfolios_.end()) return false;
    return it->second.locked.load(std::memory_order_acquire);
}

void PortfolioManager::unlock_account(uint64_t account_id) {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    auto it = portfolios_.find(account_id);
    if (it != portfolios_.end()) it->second.locked.store(false, std::memory_order_release);
}

void PortfolioManager::update_market_prices(const std::unordered_map<uint64_t, int64_t>& prices) {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    last_market_prices_ = prices;
    for (auto &p : portfolios_) p.second.recalculate_total_value(prices);
}

int64_t PortfolioManager::get_account_equity(uint64_t account_id) const {
    std::lock_guard<std::mutex> g(portfolios_mutex_);
    auto it = portfolios_.find(account_id);
    if (it == portfolios_.end()) return 0;
    return it->second.total_value.load(std::memory_order_acquire);
}

} // namespace trading::portfolio
