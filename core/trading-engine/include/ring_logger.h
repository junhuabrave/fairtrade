#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>
#include <thread>
#include <fstream>
#include <string>
#include <functional>
#include <array>

namespace trading::log {

static constexpr size_t RING_BUFFER_SIZE = 1ULL << 20;
static constexpr size_t CACHE_LINE_SIZE = 64;
static constexpr size_t MAX_LOG_ENTRY_SIZE = 256;

enum class LogLevel : uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

#pragma pack(push, 1)
struct LogEntry {
    uint64_t timestamp_ns;
    uint64_t sequence;
    uint32_t length;
    uint8_t level;
    uint8_t data[MAX_LOG_ENTRY_SIZE];
    
    size_t total_size() const noexcept { return sizeof(uint64_t) * 2 + sizeof(uint32_t) + sizeof(uint8_t) + length; }
};
#pragma pack(pop)

static_assert(sizeof(LogEntry) == 8 + 8 + 4 + 1 + 256, "LogEntry size mismatch");

class alignas(CACHE_LINE_SIZE) RingLogger {
public:
    explicit RingLogger(const std::string& binlog_path);
    ~RingLogger();
    
    RingLogger(const RingLogger&) = delete;
    RingLogger& operator=(const RingLogger&) = delete;
    RingLogger(RingLogger&&) = delete;
    RingLogger& operator=(RingLogger&&) = delete;
    
    bool initialize() noexcept;
    void shutdown() noexcept;
    
    uint64_t log(LogLevel level, const uint8_t* data, uint32_t len) noexcept;
    uint64_t log_order_new(const uint8_t* sbe_data, size_t len) noexcept;
    uint64_t log_execution(const uint8_t* sbe_data, size_t len) noexcept;
    uint64_t log_risk_event(const uint8_t* sbe_data, size_t len) noexcept;
    
    uint64_t get_write_sequence() const noexcept { return write_seq_.load(std::memory_order_acquire); }
    uint64_t get_flush_sequence() const noexcept { return flush_seq_.load(std::memory_order_acquire); }
    size_t get_buffer_utilization() const noexcept;
    
private:
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> write_seq_{0};
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> flush_seq_{0};
    alignas(CACHE_LINE_SIZE) std::atomic<bool> running_{false};
    
    std::array<uint8_t, RING_BUFFER_SIZE> buffer_;
    std::string binlog_path_;
    std::ofstream binlog_file_;
    std::thread flush_thread_;
    
    size_t sequence_to_offset(uint64_t seq) const noexcept {
        return (seq % RING_BUFFER_SIZE);
    }
    
    void flush_worker() noexcept;
    bool write_entry_to_disk(const LogEntry& entry) noexcept;
    size_t available_space() const noexcept;
    void wait_for_space(size_t required) noexcept;
};

inline uint64_t get_timestamp_ns() noexcept {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
}

} // namespace trading::log
