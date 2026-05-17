import fs from "fs";
import path from "path";

const DOCS_DIR = path.join(process.cwd(), "public", "docs");

export interface DocFile {
  slug: string;
  title: string;
  content: string;
}

export function getAllDocs(): DocFile[] {
  const files = fs.readdirSync(DOCS_DIR);
  return files
    .filter((f) => f.endsWith(".md"))
    .map((f) => {
      const slug = f.replace(".md", "");
      const content = fs.readFileSync(path.join(DOCS_DIR, f), "utf-8");
      const title = extractTitle(content) || slug;
      return { slug, title, content };
    });
}

export function getDocBySlug(slug: string): DocFile | null {
  try {
    const filePath = path.join(DOCS_DIR, `${slug}.md`);
    const content = fs.readFileSync(filePath, "utf-8");
    const title = extractTitle(content) || slug;
    return { slug, title, content };
  } catch {
    return null;
  }
}

function extractTitle(md: string): string | null {
  const match = md.match(/^#\s+(.+)/m);
  return match ? match[1].trim() : null;
}
