"use client";

import { useEffect } from "react";
import Link from "next/link";
import Aurora from "@/components/Aurora";
import Ticker from "@/components/Ticker";
import InstallBlock from "@/components/InstallBlock";

function FadeSection({ children, className = "" }: { children: React.ReactNode; className?: string }) {
  useEffect(() => {
    const io = new IntersectionObserver(
      (entries) => {
        entries.forEach((e) => {
          if (e.isIntersecting) e.target.classList.add("in");
        });
      },
      { threshold: 0.08 }
    );
    const els = document.querySelectorAll(".fade-up");
    els.forEach((el) => io.observe(el));
    return () => io.disconnect();
  }, []);

  return <div className={className}>{children}</div>;
}

function copyInstall() {
  navigator.clipboard.writeText("curl -fsSL razen.dev/install | sh").catch(() => {});
}

export default function Home() {
  return (
    <>
      {/* Hero */}
      <section className="hero">
        <Aurora />
        <div className="hero-grid" />
        <div className="hero-content">
          <div className="hero-tag">
            <span className="tag-dot" />
            razen — experimental · lexer, parser & semantic analyzer complete
          </div>
          <h1 className="hero-title">
            <span className="title-dim">the language</span>
            <br />
            <span className="title-rb">built different</span>
          </h1>
          <p className="hero-sub">
            Razen is a systems language: no hidden allocations, no implicit casts, predictable control flow, compile-time evaluation. The code you write is the code that executes.
          </p>
          <div className="terminal-install" onClick={copyInstall}>
            <span className="prompt">$</span>
            <span className="cmd">curl -fsSL razen.dev/install | sh</span>
            <span className="copy-hint">click to copy</span>
          </div>
          <div className="hero-actions">
            <Link href="/docs" className="btn-hero primary">→ read the docs</Link>
            <a href="#" className="btn-hero">try in browser</a>
            <a href="https://github.com" className="btn-hero">★ star on github</a>
          </div>
        </div>
        <div className="hero-scroll">
          <div className="scroll-line" />
        </div>
      </section>

      {/* Ticker */}
      <Ticker />

      {/* Features */}
      <FadeSection>
        <section className="section">
          <div className="container">
            <div className="fade-up">
              <div className="sec-label">// features</div>
              <h2 className="sec-title">what makes razen, razen.</h2>
              <p className="sec-sub">for engineers who want Rust's safety without the borrow checker, Zig's explicitness with more convenience, C's performance without the footguns.</p>
            </div>
            <div className="features-grid fade-up">
              <Feature icon={<LightningIcon />} name="compile-time everything" desc="Type checking, memory analysis, null safety — all resolved before a single byte runs. <span style={{color:'var(--fg-mute)'}}>const</span> and <span style={{color:'var(--fn)'}}>const func</span> evaluate at comptime. Zero runtime metaprogramming." />
              <Feature icon={<GridIcon />} name="algebraic data types" desc="Unions, structs, enums — sum and product types as first-class citizens. Exhaustive pattern matching with <span style={{color:'var(--kw)'}}>match</span>. Model your domain precisely." />
              <Feature icon={<ShieldIcon />} name="no nulls. ever." desc="Null is replaced by <span style={{color:'var(--ty)'}}>?T</span> optionals. Nullable pointers are <span style={{color:'var(--ty)'}}>?*T</span>. The compiler enforces every unwrap. No null pointer exceptions, by construction." />
              <Feature icon={<CpuIcon />} name="native performance" desc="Compiles to optimized machine code via LLVM. No VM, no JIT, no GC pauses. Zero-cost abstractions — high-level code emits the instructions you'd write by hand." />
              <Feature icon={<PackageIcon />} name="raze package manager" desc="Single binary. Add packages with <span style={{color:'var(--green)'}}>raze add</span>, build with <span style={{color:'var(--green)'}}>raze build --release</span>. Deterministic builds. Lockfiles that actually work." />
              <Feature icon={<CodeIcon />} name="readable syntax" desc="Every construct has exactly one job. No ambiguous syntax, no context-dependent behavior. <span style={{color:'var(--kw)'}}>func</span> <span style={{color:'var(--kw)'}}>ret</span> <span style={{color:'var(--kw)'}}>match</span> — words that mean what they say." />
            </div>
          </div>
        </section>
      </FadeSection>

      <div className="divider" />

      {/* Syntax + Info cards */}
      <FadeSection>
        <section className="section">
          <div className="container">
            <div className="fade-up">
              <div className="sec-label">// syntax</div>
              <h2 className="sec-title">code that reads like thought.</h2>
              <p className="sec-sub" style={{ maxWidth: 520 }}>Every construct has one job. <span style={{ color: 'var(--kw)' }}>func</span> declares. <span style={{ color: 'var(--kw)' }}>ret</span> returns. <span style={{ color: 'var(--kw)' }}>match</span> is exhaustive. <span style={{ color: 'var(--kw)' }}>:=</span> infers. No surprises.</p>
            </div>
            <div className="code-layout fade-up">
              <div className="terminal-window">
                <div className="term-bar">
                  <span className="term-dot td-r" />
                  <span className="term-dot td-y" />
                  <span className="term-dot td-g" />
                  <span className="term-title">main.rz — razen</span>
                </div>
                <div className="term-body">
                  <CodeLine n={1}><span className="cm">// Razen — unions, matching, error handling</span></CodeLine>
                  <CodeLine n={2} />
                  <CodeLine n={3}><span className="kw">use</span> <span className="sc">fmt</span></CodeLine>
                  <CodeLine n={4} />
                  <CodeLine n={5}><span className="kw">union</span> <span className="ty">Shape</span> <span className="sc">{`{`}</span></CodeLine>
                  <CodeLine n={6}>&nbsp;&nbsp;<span className="ty">Circle</span><span className="sc">(</span><span className="ty">f64</span><span className="sc">),</span></CodeLine>
                  <CodeLine n={7}>&nbsp;&nbsp;<span className="ty">Rect</span><span className="sc">(</span><span className="ty">f64</span><span className="sc">, </span><span className="ty">f64</span><span className="sc">),</span></CodeLine>
                  <CodeLine n={8}><span className="sc">{`}`}</span></CodeLine>
                  <CodeLine n={9} />
                  <CodeLine n={10}><span className="kw">func</span> <span className="fn">area</span><span className="sc">(s: </span><span className="ty">Shape</span><span className="sc">) {`->`} </span><span className="ty">f64</span> <span className="sc">{`{`}</span></CodeLine>
                  <CodeLine n={11}>&nbsp;&nbsp;<span className="kw">match</span> <span className="sc">s {`{`}</span></CodeLine>
                  <CodeLine n={12}>&nbsp;&nbsp;&nbsp;&nbsp;<span className="ty">Circle</span><span className="sc">(r) {`=>`} </span><span className="num">3.14159</span><span className="sc"> * r * r,</span></CodeLine>
                  <CodeLine n={13}>&nbsp;&nbsp;&nbsp;&nbsp;<span className="ty">Rect</span><span className="sc">(w, h) {`=>`} w * h,</span></CodeLine>
                  <CodeLine n={14}>&nbsp;&nbsp;<span className="sc">{`}`}</span></CodeLine>
                  <CodeLine n={15}><span className="sc">{`}`}</span></CodeLine>
                  <CodeLine n={16} />
                  <CodeLine n={17}><span className="kw">error</span> <span className="ty">AppError</span> <span className="sc">{`{`} </span><span className="ty">InvalidInput</span><span className="sc">, </span><span className="ty">Overflow</span><span className="sc"> {'}'}</span></CodeLine>
                  <CodeLine n={18} />
                  <CodeLine n={19}><span className="kw">func</span> <span className="fn">main</span><span className="sc">() {`->`} </span><span className="ty">AppError</span><span className="sc">!</span><span className="ty">void</span> <span className="sc">{`{`}</span></CodeLine>
                  <CodeLine n={20}>&nbsp;&nbsp;<span className="sc">c := </span><span className="ty">Shape</span><span className="sc">.</span><span className="ty">Circle</span><span className="sc">(</span><span className="num">5.0</span><span className="sc">)</span></CodeLine>
                  <CodeLine n={21}>&nbsp;&nbsp;<span className="fn">fmt.println</span><span className="sc">(</span><span className="str">"area = {}"</span><span className="sc">, .{`{`}area(c){`}`})</span></CodeLine>
                  <CodeLine n={22}>&nbsp;&nbsp;<span className="kw">ret</span></CodeLine>
                  <CodeLine n={23}><span className="sc">{`}`}</span></CodeLine>
                </div>
              </div>

              <div className="info-cards">
                <InfoCard icon={<LayersIcon />} title="exhaustive matching" desc='The compiler verifies every branch is handled. Add a union variant, the compiler flags every unhandled <span style={{color:"var(--fn)"}}>match</span> site.' />
                <InfoCard icon={<InfoIcon />} title="errors are values" desc='No exceptions. <span style={{color:"var(--ty)"}}>Error!T</span> forces callers to handle failure. <span style={{color:"var(--kw)"}}>try</span> propagates, <span style={{color:"var(--kw)"}}>catch</span> handles. No hidden control flow.' />
                <InfoCard icon={<TuneIcon />} title="inferred, not guessed" desc='Type inference flows through the whole program. Annotate where it matters with <span style={{color:"var(--ty)"}}>name : Type = expr</span>. Omit where it&apos;s obvious.' />
                <InfoCard icon={<SunIcon />} title="explicit allocation" desc='No GC, no hidden <span style={{color:"var(--fn)"}}>malloc</span>. Every heap op takes an allocator parameter: <span style={{color:"var(--ty)"}}>@arena</span> <span style={{color:"var(--ty)"}}>@pool</span> <span style={{color:"var(--ty)"}}>@fixed</span>. You decide where memory lives.' />
              </div>
            </div>
          </div>
        </section>
      </FadeSection>

      {/* Stats */}
      <div className="stats-strip">
        <div className="stats-inner fade-up">
          <Stat n="0" l="// hidden allocs" />
          <Stat n="0" l="// runtime exceptions" />
          <Stat n="32k" l="// max int width (bits)" />
          <Stat n="100%" l="// explicit type coverage" />
        </div>
      </div>

      <div className="divider" />

      {/* Why section */}
      <FadeSection>
        <section className="section">
          <div className="container">
            <div className="fade-up">
              <div className="sec-label">// comparison</div>
              <h2 className="sec-title">razen vs. the field.</h2>
              <p className="sec-sub">not another clone. a synthesis of what works, none of what doesn't.</p>
            </div>
            <div className="why-grid fade-up">
              <WhyCard lang="razen" langColor="var(--rb5)" fill="rb" ratings={[["speed", 96], ["safety", 98], ["dx", 94], ["ergonomics", 95]]} />
              <WhyCard lang="rust" fill="a" ratings={[["speed", 98], ["safety", 99], ["dx", 58], ["ergonomics", 62]]} />
              <WhyCard lang="go" fill="g" ratings={[["speed", 82], ["safety", 70], ["dx", 86], ["ergonomics", 80]]} />
              <WhyCard lang="zig" fill="b" ratings={[["speed", 97], ["safety", 80], ["dx", 72], ["ergonomics", 68]]} />
            </div>
          </div>
        </section>
      </FadeSection>

      <div className="divider" />

      {/* Install section */}
      <FadeSection>
        <section className="section">
          <div className="container">
            <div className="fade-up">
              <div className="sec-label">// install</div>
              <h2 className="sec-title">one command. you're in.</h2>
              <p className="sec-sub">works on linux, macos, windows. single binary. no dependencies.</p>
            </div>
            <div className="fade-up" style={{ display: "flex", justifyContent: "center" }}>
              <InstallBlock />
            </div>
          </div>
        </section>
      </FadeSection>

      {/* CTA */}
      <section className="cta-section fade-up">
        <div className="cta-glow" />
        <div className="rb-bar" style={{ position: "absolute", top: 0, left: 0, right: 0 }} />
        <h2 className="cta-title">write the future.<span className="cur" /></h2>
        <p className="cta-sub">razen is free, open source, and early. lexer · parser · semantic analyzer — complete. llvm codegen — in progress.</p>
        <div className="cta-actions">
          <Link href="/docs" className="btn-hero primary">→ get started</Link>
          <Link href="/roadmap" className="btn-hero">read the spec</Link>
          <a href="#" className="btn-hero">join discord</a>
        </div>
      </section>
    </>
  );
}

/* ─── Sub-components ─── */

function CodeLine({ n, children }: { n: number; children?: React.ReactNode }) {
  return (
    <div className="line">
      <span className="lnum">{n}</span>
      {children}
    </div>
  );
}

function Feature({ icon, name, desc }: { icon: React.ReactNode; name: string; desc: string }) {
  return (
    <div className="feat">
      <div className="feat-icon">{icon}</div>
      <div className="feat-name">{name}</div>
      <div className="feat-desc" dangerouslySetInnerHTML={{ __html: desc }} />
    </div>
  );
}

function InfoCard({ icon, title, desc }: { icon: React.ReactNode; title: string; desc: string }) {
  return (
    <div className="info-card">
      <div className="ic-icon">{icon}</div>
      <div>
        <div className="ic-title">{title}</div>
        <div className="ic-desc" dangerouslySetInnerHTML={{ __html: desc }} />
      </div>
    </div>
  );
}

function Stat({ n, l }: { n: string; l: string }) {
  return (
    <div className="stat">
      <span className="stat-n">{n}</span>
      <div className="stat-l">{l}</div>
    </div>
  );
}

function WhyCard({ lang, langColor, fill, ratings }: { lang: string; langColor?: string; fill: string; ratings: [string, number][] }) {
  return (
    <div className="why-card">
      <div className="why-lang" style={langColor ? { color: langColor } : undefined}>// {lang}</div>
      <div className="why-rating">
        {ratings.map(([label, val]) => (
          <div key={label} className="wr-row">
            <span className="wr-label">{label}</span>
            <div className="wr-bar">
              <div className={`wr-fill fill-${fill}`} style={{ width: `${val}%` }} />
            </div>
            <span className="wr-val">{val}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

/* ─── SVG Icons ─── */

function LightningIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <polyline points="16 3 8 13 12 13 8 21" />
    </svg>
  );
}
function GridIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <rect x="3" y="3" width="7" height="7" />
      <rect x="14" y="3" width="7" height="7" />
      <rect x="14" y="14" width="7" height="7" />
      <rect x="3" y="14" width="7" height="7" />
    </svg>
  );
}
function ShieldIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" />
    </svg>
  );
}
function CpuIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <rect x="4" y="4" width="16" height="16" rx="2" />
      <rect x="9" y="9" width="6" height="6" />
      <line x1="9" y1="1" x2="9" y2="4" />
      <line x1="15" y1="1" x2="15" y2="4" />
      <line x1="9" y1="20" x2="9" y2="23" />
      <line x1="15" y1="20" x2="15" y2="23" />
      <line x1="20" y1="9" x2="23" y2="9" />
      <line x1="20" y1="15" x2="23" y2="15" />
      <line x1="1" y1="9" x2="4" y2="9" />
      <line x1="1" y1="15" x2="4" y2="15" />
    </svg>
  );
}
function PackageIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <line x1="16.5" y1="9.4" x2="7.5" y2="4.21" />
      <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z" />
      <polyline points="3.27 6.96 12 12.01 20.73 6.96" />
      <line x1="12" y1="22.08" x2="12" y2="12" />
    </svg>
  );
}
function CodeIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <polyline points="16 18 22 12 16 6" />
      <polyline points="8 6 2 12 8 18" />
    </svg>
  );
}
function LayersIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <path d="M9 3H5a2 2 0 0 0-2 2v4m6-6h10a2 2 0 0 1 2 2v4M9 3v18m0 0h10a2 2 0 0 0 2-2v-4M9 21H5a2 2 0 0 1-2-2v-4m0 0h18" />
    </svg>
  );
}
function InfoIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <circle cx="12" cy="12" r="10" />
      <line x1="12" y1="8" x2="12" y2="12" />
      <line x1="12" y1="16" x2="12.01" y2="16" />
    </svg>
  );
}
function TuneIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <path d="M14.5 10c-.83 0-1.5-.67-1.5-1.5v-5c0-.83.67-1.5 1.5-1.5s1.5.67 1.5 1.5v5c0 .83-.67 1.5-1.5 1.5z" />
      <path d="M20.5 10H19V8.5c0-.83.67-1.5 1.5-1.5s1.5.67 1.5 1.5-.67 1.5-1.5 1.5z" />
      <path d="M9.5 14c.83 0 1.5.67 1.5 1.5v5c0 .83-.67 1.5-1.5 1.5S8 21.33 8 20.5v-5c0-.83.67-1.5 1.5-1.5z" />
      <path d="M3.5 14H5v1.5c0 .83-.67 1.5-1.5 1.5S2 16.33 2 15.5 2.67 14 3.5 14z" />
      <path d="M14 14.5c0-.83.67-1.5 1.5-1.5h5c.83 0 1.5.67 1.5 1.5s-.67 1.5-1.5 1.5h-5c-.83 0-1.5-.67-1.5-1.5z" />
      <path d="M15.5 9H14V7.5" />
      <path d="M10 9.5c0 .83-.67 1.5-1.5 1.5H3c-.83 0-1.5-.67-1.5-1.5S2.17 8 3 8h5.5c.83 0 1.5.67 1.5 1.5z" />
    </svg>
  );
}
function SunIcon() {
  return (
    <svg viewBox="0 0 24 24">
      <circle cx="12" cy="12" r="3" />
      <path d="M12 1v4M12 19v4M4.22 4.22l2.83 2.83M16.95 16.95l2.83 2.83M1 12h4M19 12h4M4.22 19.78l2.83-2.83M16.95 7.05l2.83-2.83" />
    </svg>
  );
}
