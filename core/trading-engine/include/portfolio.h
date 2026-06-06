#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

namespace trading::portfolio {

struct Position {
    alignas(64) std::atomic<int64_t> quantity{0};
    alignas(64) std::atomic<int64_t> avg_cost_basis{0};
    alignas(64) std::atomic<int64_t> total_cost{0};
    alignas(64) std::atomic<int64_t> realized_pnl{0};
    alignas(64) std::atomic<int64_t> unrealized_pnl{0};
    alignas(64) std::atomic<uint32_t> trade_count{0};
    alignas(64) std::atomic<uint64_t> last_update_ns{0};
    
    void update_on_fill(int64_t fill_qty, int64_t fill_price, uint64_t timestamp_ns) noexcept;
    int64_t get_market_value(int64_t current_price) const noexcept;
    void mark_to_market(int64_t current_price) noexcept;
};

// Plain-data snapshot of a Position suitable for returning/copying
struct PositionSnapshot {
    int64_t quantity = 0;
    int64_t avg_cost_basis = 0;
    int64_t total_cost = 0;
    int64_t realized_pnl = 0;
    int64_t unrealized_pnl = 0;
    uint32_t trade_count = 0;
    uint64_t last_update_ns = 0;
};

struct AccountPortfolio {
    uint64_t account_id;
    std::unordered_map<uint64_t, Position> positions;
    alignas(64) std::atomic<int64_t> total_value{0};
    alignas(64) std::atomic<int64_t> buying_power{10000000000};
    alignas(64) std::atomic<bool> locked{false};
    alignas(64) std::atomic<uint64_t> last_signoff_ns{0};
    
    void apply_fill(uint64_t symbol_id, int64_t qty, int64_t price, uint64_t timestamp_ns);
    int64_t get_total_position_value(uint64_t symbol_id) const noexcept;
    void recalculate_total_value(const std::unordered_map<uint64_t, int64_t>& market_prices);
};

// Plain-data snapshot of AccountPortfolio for safe return over API boundaries
struct AccountPortfolioSnapshot {
    uint64_t account_id = 0;
    std::unordered_map<uint64_t, PositionSnapshot> positions;
    int64_t total_value = 0;
    int64_t buying_power = 0;
    bool locked = false;
    uint64_t last_signoff_ns = 0;
};

class PortfolioManager {
public:
    static constexpr uint64_t SECURE_SIGNOFF_2026 = 0x2026DEADBEEFCAFEULL;
    
    PortfolioManager() = default;
    ~PortfolioManager() = default;
    
    PortfolioManager(const PortfolioManager&) = delete;
    PortfolioManager& operator=(const PortfolioManager&) = delete;
    PortfolioManager(PortfolioManager&&) = delete;
    PortfolioManager& operator=(PortfolioManager&&) = delete;
    
    void process_execution(uint64_t account_id, uint64_t symbol_id, int64_t qty, 
                          int64_t price, uint64_t timestamp_ns);
    
    // Return copies as plain-data snapshots to avoid atomic copy/move issues
    PositionSnapshot get_position(uint64_t account_id, uint64_t symbol_id) const;
    AccountPortfolioSnapshot get_account_summary(uint64_t account_id) const;
    std::vector<uint64_t> get_all_accounts() const;
    
    bool signoff_and_lock(uint64_t account_id, uint64_t signoff_key) noexcept;
    bool is_account_locked(uint64_t account_id) const noexcept;
    void unlock_account(uint64_t account_id);
    
    void update_market_prices(const std::unordered_map<uint64_t, int64_t>& prices);
    int64_t get_account_equity(uint64_t account_id) const;
    
private:
    mutable std::mutex portfolios_mutex_;
    std::unordered_map<uint64_t, AccountPortfolio> portfolios_;
    std::unordered_map<uint64_t, int64_t> last_market_prices_;
    
    AccountPortfolio& get_or_create_portfolio(uint64_t account_id);
};

inline int64_t calculate_vwap(int64_t total_cost, int64_t total_qty) noexcept {
    if (total_qty == 0) return 0;
    return total_cost / total_qty;
}

} // namespace trading::portfolio
