"use client";

import { useState } from "react";

const tabs = [
  { id: "unix", label: "unix / macos" },
  { id: "win", label: "windows" },
  { id: "cargo", label: "raze" },
];

const tabContent: Record<string, { prompt: string; promptStyle?: React.CSSProperties; lines: { text: string; copy?: string; indent?: string }[] }> = {
  unix: {
    prompt: "$",
    lines: [
      { text: "curl -fsSL razen.dev/install | sh", copy: "curl -fsSL razen.dev/install | sh" },
      { text: "razen --version", copy: "razen --version" },
      { text: "razen 0.1.62 (stable)", indent: "18px" },
    ],
  },
  win: {
    prompt: "PS>",
    promptStyle: { color: "var(--blue)" },
    lines: [
      { text: "irm razen.dev/install.ps1 | iex", copy: "irm razen.dev/install.ps1 | iex" },
      { text: "razen --version", copy: "razen --version" },
      { text: "razen 0.1.62 (stable)", indent: "34px" },
    ],
  },
  cargo: {
    prompt: "$",
    lines: [
      { text: "raze add http", copy: "raze add http" },
      { text: "raze build --release", copy: "raze build --release" },
      { text: "raze run", copy: "raze run" },
    ],
  },
};

export default function InstallBlock() {
  const [active, setActive] = useState("unix");
  const [copiedIdx, setCopiedIdx] = useState<string | null>(null);

  function handleCopy(text: string, key: string) {
    navigator.clipboard.writeText(text).catch(() => {});
    setCopiedIdx(key);
    setTimeout(() => setCopiedIdx(null), 2000);
  }

  return (
    <div className="install-block">
      <div className="install-tabs">
        {tabs.map((t) => (
          <div
            key={t.id}
            className={`tab ${active === t.id ? "active" : ""}`}
            onClick={() => setActive(t.id)}
          >
            {t.label}
          </div>
        ))}
      </div>
      {tabs.map((t) => (
        <div
          key={t.id}
          id={`tab-${t.id}`}
          className={`tab-content ${active === t.id ? "active" : ""}`}
        >
          <div className="install-cmd">
            {tabContent[t.id].lines.map((line, i) => (
              <div key={i} className="iline">
                {!line.indent && (
                  <span className="prompt" style={tabContent[t.id].promptStyle}>
                    {tabContent[t.id].prompt}
                  </span>
                )}
                <span
                  style={{
                    color: line.indent ? "var(--fg-mute)" : "var(--fg)",
                    paddingLeft: line.indent || undefined,
                  }}
                >
                  {line.text}
                </span>
                {line.copy && (
                  <span
                    className="icopy"
                    onClick={() => handleCopy(line.copy!, `${t.id}-${i}`)}
                  >
                    {copiedIdx === `${t.id}-${i}` ? "copied!" : "copy"}
                  </span>
                )}
              </div>
            ))}
          </div>
        </div>
      ))}
    </div>
  );
}
