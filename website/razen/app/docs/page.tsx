import { getAllDocs } from "@/lib/docs";
import DocsViewer from "@/components/DocsViewer";

export default function DocsPage() {
  const docs = getAllDocs();
  return <DocsViewer docs={docs} />;
}
