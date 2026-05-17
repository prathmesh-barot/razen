"use client";

import { useState, useMemo } from "react";
import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";
import type { DocFile } from "@/lib/docs";

interface Props {
  docs: DocFile[];
}

export default function DocsViewer({ docs }: Props) {
  const [activeSlug, setActiveSlug] = useState(docs[0]?.slug ?? "");
  const activeDoc = useMemo(
    () => docs.find((d) => d.slug === activeSlug) ?? docs[0],
    [docs, activeSlug]
  );

  return (
    <div className="docs-layout">
      <aside className="docs-sidebar">
        <div className="docs-sidebar-title">// documentation</div>
        {docs.map((doc) => (
          <div
            key={doc.slug}
            className={`docs-nav-item ${doc.slug === activeSlug ? "active" : ""}`}
            onClick={() => setActiveSlug(doc.slug)}
          >
            {doc.title}
          </div>
        ))}
      </aside>
      <main className="docs-main">
        <div className="markdown-body">
          {activeDoc && (
            <ReactMarkdown
              remarkPlugins={[remarkGfm]}
              components={{
                a: ({ href, children, ...props }) => {
                  const isExternal = href?.startsWith("http");
                  return (
                    <a
                      href={href}
                      target={isExternal ? "_blank" : undefined}
                      rel={isExternal ? "noopener noreferrer" : undefined}
                      {...props}
                    >
                      {children}
                    </a>
                  );
                },
                code: ({ className, children, ...props }) => {
                  const isInline = !className;
                  if (isInline) {
                    return <code {...props}>{children}</code>;
                  }
                  return (
                    <pre>
                      <code className={className} {...props}>
                        {children}
                      </code>
                    </pre>
                  );
                },
              }}
            >
              {activeDoc.content}
            </ReactMarkdown>
          )}
        </div>
      </main>
    </div>
  );
}
