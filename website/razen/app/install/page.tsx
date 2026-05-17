import InstallBlock from "@/components/InstallBlock";

export default function InstallPage() {
  return (
    <>
      <div className="page-hero">
        <h1>
          <span className="title-dim">install</span>{" "}
          <span className="title-rb">razen</span>
        </h1>
        <p>one command. you're in. works on linux, macos, windows. single binary. no dependencies.</p>
      </div>

      <div className="install-page-content">
        <div className="install-steps">
          <div className="install-step">
            <h3>
              <span className="step-number">1</span>
              Install the compiler
            </h3>
            <InstallBlock />
          </div>

          <div className="install-step">
            <h3>
              <span className="step-number">2</span>
              Verify it works
            </h3>
            <div className="terminal-window" style={{ marginTop: 16 }}>
              <div className="term-bar">
                <span className="term-dot td-r" />
                <span className="term-dot td-y" />
                <span className="term-dot td-g" />
                <span className="term-title">terminal</span>
              </div>
              <div className="term-body">
                <div className="line"><span className="lnum">$</span><span style={{ color: "var(--fg)" }}>razen --version</span></div>
                <div className="line"><span className="lnum" /><span style={{ color: "var(--fg-mute)" }}>razen 0.1.62 (stable)</span></div>
                <div className="line"><span className="lnum">$</span><span style={{ color: "var(--fg)" }}>razen run main.rz</span></div>
                <div className="line"><span className="lnum" /><span style={{ color: "var(--green)" }}>hello, world!</span></div>
              </div>
            </div>
          </div>

          <div className="install-step">
            <h3>
              <span className="step-number">3</span>
              Start a project
            </h3>
            <div className="terminal-window" style={{ marginTop: 16 }}>
              <div className="term-bar">
                <span className="term-dot td-r" />
                <span className="term-dot td-y" />
                <span className="term-dot td-g" />
                <span className="term-title">terminal</span>
              </div>
              <div className="term-body">
                <div className="line"><span className="lnum">$</span><span style={{ color: "var(--fg)" }}>raze init my-project</span></div>
                <div className="line"><span className="lnum">$</span><span style={{ color: "var(--fg)" }}>cd my-project</span></div>
                <div className="line"><span className="lnum">$</span><span style={{ color: "var(--fg)" }}>raze build --release</span></div>
                <div className="line"><span className="lnum">$</span><span style={{ color: "var(--green)" }}>./my-project</span></div>
              </div>
            </div>
          </div>

          <div className="install-step">
            <h3>
              <span className="step-number">4</span>
              System requirements
            </h3>
            <div className="info-card" style={{ marginTop: 16 }}>
              <div className="ic-icon">
                <svg viewBox="0 0 24 24" width="16" height="16" stroke="var(--fg-dim)" fill="none" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
                  <circle cx="12" cy="12" r="10" />
                  <line x1="12" y1="16" x2="12" y2="12" />
                  <line x1="12" y1="8" x2="12.01" y2="8" />
                </svg>
              </div>
              <div>
                <div className="ic-title">required</div>
                <div className="ic-desc">
                  Linux kernel 5.4+, macOS 12+, or Windows 10 64-bit. LLVM 16+ recommended for codegen.
                  No other dependencies — razen ships as a single static binary.
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </>
  );
}
