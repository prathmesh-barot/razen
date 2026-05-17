import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";
import fs from "fs";
import path from "path";

export default function RoadmapPage() {
  let content = "";
  try {
    content = fs.readFileSync(
      path.join(process.cwd(), "public", "docs", "roadmap.md"),
      "utf-8"
    );
  } catch {
    content = "# Roadmap\n\nRoadmap content not found.";
  }

  return (
    <>
      <div className="page-hero">
        <h1>
          <span className="title-dim">the</span>{" "}
          <span className="title-rb">roadmap</span>
        </h1>
        <p>where razen is today and where it's heading. phases 1-3 complete, phase 4 in progress.</p>
      </div>

      <div className="roadmap-section">
        <div className="container">
          <div className="markdown-body">
            <ReactMarkdown remarkPlugins={[remarkGfm]}>{content}</ReactMarkdown>
          </div>
        </div>
      </div>
    </>
  );
}
