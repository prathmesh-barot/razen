import Link from "next/link";

export default function AboutPage() {
  return (
    <>
      <div className="page-hero">
        <h1>
          <span className="title-dim">about</span>{" "}
          <span className="title-rb">razen</span>
        </h1>
        <p>a systems language built on first principles. no hidden magic. period.</p>
      </div>

      <div className="about-section">
        <div className="container">
          <div className="about-grid">
            <div className="about-card">
              <h3>what is razen?</h3>
              <p>
                Razen is a general-purpose systems programming language that prioritizes explicitness,
                safety, and predictable performance. It compiles via LLVM to native machine code with
                zero hidden runtime costs.
              </p>
              <p style={{ marginTop: 12 }}>
                Inspired by Rust's safety, Zig's control, and Go's simplicity — Razen synthesizes
                what works without the baggage. No borrow checker. No garbage collector. No exceptions.
                No null pointers. No hidden allocations.
              </p>
            </div>

            <div className="about-card">
              <h3>current status</h3>
              <p>
                The lexer, parser, and semantic analyzer are complete. LLVM codegen is in progress.
                The language is experimental and evolving — now is the perfect time to contribute
                ideas, feedback, and code.
              </p>
              <p style={{ marginTop: 12 }}>
                Razen is developed in the open under the MIT license. All design decisions are
                documented and debated publicly.
              </p>
            </div>

            <div className="about-card">
              <h3>design philosophy</h3>
              <p>
                <strong>Zero hidden magic:</strong> Every allocation, every cast, every control flow
                path is visible in the source code. What you write is what the machine does.
              </p>
              <p style={{ marginTop: 8 }}>
                <strong>Predictable performance:</strong> No GC pauses, no JIT warmup, no
                hidden reference counting. The cost model is visible in the type signatures.
              </p>
              <p style={{ marginTop: 8 }}>
                <strong>Ergonomic safety:</strong> Safety through exhaustive checking (match,
                error unions, optionals) rather than complex type system gymnastics.
              </p>
            </div>

            <div className="about-card">
              <h3>get involved</h3>
              <p>
                Razen is an open-source community effort. Whether you're interested in compiler
                development, standard library design, documentation, or just trying it out —
                there's a place for you.
              </p>
              <div style={{ marginTop: 16, display: "flex", gap: 12 }}>
                <Link href="/docs" className="btn-hero primary" style={{ display: "inline-flex" }}>
                  → read docs
                </Link>
                <a href="https://github.com" className="btn-hero" style={{ display: "inline-flex" }}>
                  github
                </a>
              </div>
            </div>
          </div>
        </div>
      </div>
    </>
  );
}
