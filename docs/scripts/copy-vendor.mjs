// Copies the browser Mermaid build from node_modules to public/, so the site
// serves its own copy instead of fetching one from a CDN.
import { copyFileSync, mkdirSync } from 'node:fs';

mkdirSync(new URL('../public/', import.meta.url), { recursive: true });
copyFileSync(
	new URL('../node_modules/mermaid/dist/mermaid.min.js', import.meta.url),
	new URL('../public/mermaid.min.js', import.meta.url),
);
