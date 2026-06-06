import React, {useEffect, useState} from "react";
import type {MarketDataTick} from "./types/trading";

export default function App() {
  const [ticks, setTicks] = useState<MarketDataTick[]>([]);

  useEffect(() => {
    let mounted = true;
    const poll = async () => {
      try {
        const res = await fetch("/api/market/ticks");
        if (!res.ok) return;
        const data = await res.json();
        if (mounted) setTicks(data.slice(0, 50));
      } catch (e) {
        // ignore
      }
    };
    const id = setInterval(poll, 500);
    poll();
    return () => { mounted = false; clearInterval(id); };
  }, []);

  return (
    <div style={{padding:20}}>
      <h2>Market Ticks</h2>
      <table>
        <thead>
          <tr><th>Symbol</th><th>Bid</th><th>Ask</th><th>Last</th><th>Vol</th></tr>
        </thead>
        <tbody>
          {ticks.map(t => (
            <tr key={t.symbol_id + "-" + t.timestamp_ns}>
              <td>{t.symbol_id}</td>
              <td>{t.bid_price}</td>
              <td>{t.ask_price}</td>
              <td>{t.last_price}</td>
              <td>{t.volume}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
