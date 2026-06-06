#include "ring_logger.h"
#include <cstring>
#include <chrono>
#include <unistd.h>

namespace trading::log {

RingLogger::RingLogger(const std::string& binlog_path)
    : binlog_path_(binlog_path)
{
}

RingLogger::~RingLogger() {
    shutdown();
}

bool RingLogger::initialize() noexcept {
    if (running_.load(std::memory_order_acquire)) return true;
    binlog_file_.open(binlog_path_, std::ios::binary | std::ios::app);
    if (!binlog_file_.is_open()) return false;
    running_.store(true, std::memory_order_release);
    flush_thread_ = std::thread(&RingLogger::flush_worker, this);
    return true;
}

void RingLogger::shutdown() noexcept {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) return;
    if (flush_thread_.joinable()) flush_thread_.join();
    if (binlog_file_.is_open()) binlog_file_.close();
}

uint64_t RingLogger::log(LogLevel level, const uint8_t* data, uint32_t len) noexcept {
    if (!running_.load(std::memory_order_acquire)) return UINT64_MAX;
    if (len > MAX_LOG_ENTRY_SIZE) return UINT64_MAX;

    uint64_t seq = write_seq_.fetch_add(1, std::memory_order_acq_rel);
    size_t offset = sequence_to_offset(seq);
    // write into buffer with simple header
    LogEntry entry{};
    entry.timestamp_ns = get_timestamp_ns();
    entry.sequence = seq;
    entry.length = len;
    entry.level = static_cast<uint8_t>(level);
    memcpy(entry.data, data, len);

    // copy raw bytes into ring buffer
    size_t total = entry.total_size();
    if (total > RING_BUFFER_SIZE) return UINT64_MAX;
    size_t end = offset + total;
    if (end <= RING_BUFFER_SIZE) {
        memcpy(&buffer_[offset], &entry, total);
    } else {
        size_t first = RING_BUFFER_SIZE - offset;
        memcpy(&buffer_[offset], &entry, first);
        memcpy(&buffer_[0], reinterpret_cast<const uint8_t*>(&entry) + first, total - first);
    }

    // advance flush sequence lazily
    // background flush thread will observe write_seq_
    return seq;
}

uint64_t RingLogger::log_order_new(const uint8_t* sbe_data, size_t len) noexcept {
    return log(LogLevel::INFO, sbe_data, static_cast<uint32_t>(len));
}

uint64_t RingLogger::log_execution(const uint8_t* sbe_data, size_t len) noexcept {
    return log(LogLevel::DEBUG, sbe_data, static_cast<uint32_t>(len));
}

uint64_t RingLogger::log_risk_event(const uint8_t* sbe_data, size_t len) noexcept {
    return log(LogLevel::WARN, sbe_data, static_cast<uint32_t>(len));
}

size_t RingLogger::get_buffer_utilization() const noexcept {
    uint64_t w = write_seq_.load(std::memory_order_acquire);
    uint64_t f = flush_seq_.load(std::memory_order_acquire);
    return static_cast<size_t>((w - f) % RING_BUFFER_SIZE);
}

void RingLogger::flush_worker() noexcept {
    while (running_.load(std::memory_order_acquire)) {
        uint64_t f = flush_seq_.load(std::memory_order_acquire);
        uint64_t w = write_seq_.load(std::memory_order_acquire);
        if (f == w) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        size_t offset = sequence_to_offset(f);
        // read a single entry header to determine size (safe because producer wrote full entry)
        LogEntry entry{};
        size_t header_size = sizeof(LogEntry);
        if (offset + header_size <= RING_BUFFER_SIZE) {
            memcpy(&entry, &buffer_[offset], header_size);
        } else {
            size_t first = RING_BUFFER_SIZE - offset;
            memcpy(&entry, &buffer_[offset], first);
            memcpy(reinterpret_cast<uint8_t*>(&entry) + first, &buffer_[0], header_size - first);
        }

        // write the entire fixed-size structure to disk
        if (binlog_file_.is_open()) {
            binlog_file_.write(reinterpret_cast<const char*>(&entry), sizeof(LogEntry));
            binlog_file_.flush();
        }

        flush_seq_.fetch_add(1, std::memory_order_acq_rel);
    }
}

bool RingLogger::write_entry_to_disk(const LogEntry& /*entry*/) noexcept {
    // not used in this simplified implementation
    return true;
}

size_t RingLogger::available_space() const noexcept {
    return RING_BUFFER_SIZE - get_buffer_utilization();
}

void RingLogger::wait_for_space(size_t /*required*/) noexcept {
    // simple busy-wait/backoff; low-frequency in this simplified impl
    while (available_space() == 0 && running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

} // namespace trading::log
