const tickerItems = [
  "no hidden allocs", "no implicit casts", "compiles via llvm",
  "union types", "exhaustive match", "no garbage collector",
  "behaviours (traits)", "error!T unions", "explicit allocators",
  "comptime evaluation",
];

export default function Ticker() {
  return (
    <div className="ticker-wrap">
      <div className="ticker">
        {[...tickerItems, ...tickerItems].map((item, i) => (
          <span key={i} className="ticker-item">
            {item} <span>·</span>
          </span>
        ))}
      </div>
    </div>
  );
}
