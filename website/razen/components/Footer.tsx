import Link from "next/link";

export default function Footer() {
  return (
    <footer>
      <div className="footer-inner">
        <div className="footer-brand">
          <div className="nav-logo" style={{ display: "inline-flex" }}>
            <span className="logo-mark">Rz</span> razen
          </div>
          <p>meaningful, accurate, simple, maximum performance.</p>
        </div>
        <div className="f-col">
          <h4>// learn</h4>
          <ul>
            <li><Link href="/docs">quickstart</Link></li>
            <li><Link href="/docs">language guide</Link></li>
            <li><Link href="/docs">std library</Link></li>
            <li><Link href="/docs">examples</Link></li>
          </ul>
        </div>
        <div className="f-col">
          <h4>// tools</h4>
          <ul>
            <li><a href="#">playground</a></li>
            <li><a href="#">lsp / editor</a></li>
            <li><a href="#">raze cli</a></li>
            <li><a href="#">formatter</a></li>
          </ul>
        </div>
        <div className="f-col">
          <h4>// community</h4>
          <ul>
            <li><a href="https://github.com">github</a></li>
            <li><a href="#">discord</a></li>
            <li><a href="#">rfcs</a></li>
            <li><a href="#">contribute</a></li>
          </ul>
        </div>
      </div>
      <div className="footer-bottom">
        <span className="footer-copy">&copy; 2026 razen lang &mdash; MIT license</span>
        <span className="footer-copy" style={{ color: "var(--fg-mute)" }}>made with <span style={{ color: "var(--red)" }}>&hearts;</span> and too much coffee</span>
      </div>
    </footer>
  );
}
