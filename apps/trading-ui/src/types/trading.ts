export interface OrderNew {
  account_id: number;
  order_id: number;
  timestamp_ns: number;
  symbol_id: number;
  price: number;
  quantity: number;
  side: "buy" | "sell";
}

export interface MarketDataTick {
  symbol_id: number;
  timestamp_ns: number;
  bid_price: number;
  ask_price: number;
  bid_size: number;
  ask_size: number;
  last_price: number;
  volume: number;
}

export interface ExecutionReport {
  account_id: number;
  order_id: number;
  exec_id: number;
  timestamp_ns: number;
  symbol_id: number;
  price: number;
  executed_qty: number;
  leaves_qty: number;
}
