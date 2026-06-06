#pragma once

#include <cstdint>
#include <cstring>
#include <array>

#pragma pack(push, 1)

namespace trading::sbe {

static constexpr uint16_t SBE_MESSAGE_HEADER_SIZE = 8;
static constexpr uint16_t SBE_BLOCK_LENGTH = 64;
static constexpr uint8_t SBE_SCHEMA_VERSION = 1;

enum class MessageType : uint16_t {
    HEARTBEAT = 0,
    ORDER_NEW = 1,
    ORDER_CANCEL = 2,
    ORDER_REPLACE = 3,
    EXECUTION_REPORT = 4,
    MARKET_DATA = 5,
    RISK_REJECT = 99
};

struct SbeMessageHeader {
    uint16_t block_length;
    uint16_t template_id;
    uint16_t schema_id;
    uint16_t version;
};

struct OrderNewMessage {
    static constexpr MessageType TYPE = MessageType::ORDER_NEW;
    static constexpr uint16_t BLOCK_LENGTH = 64;
    
    uint64_t account_id;
    uint64_t order_id;
    uint64_t timestamp_ns;
    uint64_t symbol_id;
    int64_t price;
    uint32_t quantity;
    uint8_t side;
    uint8_t order_type;
    uint8_t time_in_force;
    uint8_t reserved[27];
    
    bool validate() const noexcept {
        return account_id != 0 && order_id != 0 && quantity > 0 && quantity <= 1000000000;
    }
};

struct OrderCancelMessage {
    static constexpr MessageType TYPE = MessageType::ORDER_CANCEL;
    static constexpr uint16_t BLOCK_LENGTH = 32;
    
    uint64_t account_id;
    uint64_t order_id;
    uint64_t timestamp_ns;
    uint64_t orig_order_id;
};

struct ExecutionReportMessage {
    static constexpr MessageType TYPE = MessageType::EXECUTION_REPORT;
    static constexpr uint16_t BLOCK_LENGTH = 64;
    
    uint64_t account_id;
    uint64_t order_id;
    uint64_t exec_id;
    uint64_t timestamp_ns;
    uint64_t symbol_id;
    int64_t price;
    uint32_t executed_qty;
    uint32_t leaves_qty;
    uint8_t exec_type;
    uint8_t side;
    uint8_t reserved[14];
};

struct MarketDataTick {
    static constexpr MessageType TYPE = MessageType::MARKET_DATA;
    static constexpr uint16_t BLOCK_LENGTH = 48;
    
    uint64_t symbol_id;
    uint64_t timestamp_ns;
    int64_t bid_price;
    int64_t ask_price;
    uint32_t bid_size;
    uint32_t ask_size;
    uint32_t last_price;
    uint32_t volume;
};

struct RiskRejectMessage {
    static constexpr MessageType TYPE = MessageType::RISK_REJECT;
    static constexpr uint16_t BLOCK_LENGTH = 32;
    
    uint64_t account_id;
    uint64_t order_id;
    uint64_t timestamp_ns;
    int32_t reject_code;
    uint8_t reason[12];
};

template<typename T>
const T* cast_message(const uint8_t* data, size_t len) noexcept {
    if (len < sizeof(T)) return nullptr;
    return reinterpret_cast<const T*>(data);
}

template<typename T>
T* cast_message(uint8_t* data, size_t len) noexcept {
    if (len < sizeof(T)) return nullptr;
    return reinterpret_cast<T*>(data);
}

} // namespace trading::sbe

#pragma pack(pop)
