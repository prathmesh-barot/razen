import Link from "next/link";

export default function Nav() {
  return (
    <>
      <div className="rb-bar" />
      <nav>
        <Link href="/" className="nav-logo">
          <span className="logo-mark">Rz</span>
          razen
        </Link>
        <ul className="nav-links">
          <li><Link href="/docs">docs</Link></li>
          <li><Link href="/install">install</Link></li>
          <li><Link href="/roadmap">roadmap</Link></li>
          <li><Link href="/about">about</Link></li>
        </ul>
        <div className="nav-right">
          <Link href="https://github.com" className="btn-sm">github</Link>
          <Link href="/docs" className="btn-sm primary">get started</Link>
        </div>
      </nav>
    </>
  );
}
