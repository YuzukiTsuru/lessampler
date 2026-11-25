// The Markdown documentation lives in ../docs/. Starlight expects content
// under src/content/docs/, so this script copies the pages there, strips the
// leading H1 (it becomes the Starlight title) and adds frontmatter.
// Runs before `astro dev` and `astro build`.
import { cpSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const siteRoot = resolve(here, '..');
const repoRoot = resolve(siteRoot, '..');
const sourceDirectory = join(repoRoot, 'docs');
const outputDirectory = join(siteRoot, 'src/content/docs');
const assetsDirectory = join(siteRoot, 'src/assets');

const pages = [
	{
		source: 'build.md',
		target: 'build.md',
		title: 'Build',
		description: 'Requirements, presets, optional backends, packaging and tests.',
	},
	{
		source: 'configuration.md',
		target: 'configuration.md',
		title: 'Configuration',
		description: 'lessampler.less, model caches and performance.',
	},
	{
		source: 'rendering.md',
		target: 'rendering.md',
		title: 'Rendering a UST',
		description: 'lesswavtool, the UST host, placement and envelopes.',
	},
	{
		source: 'neural.md',
		target: 'neural.md',
		title: 'Neural backends',
		description: 'NHVSing, whole-song polish and feature-level assembly.',
	},
	{
		source: 'quality.md',
		target: 'quality.md',
		title: 'Quality evaluation',
		description: 'SingMOS-Pro and objective metrics with a versioned JSON report.',
	},
	{
		source: 'architecture-details.md',
		target: 'architecture-details.md',
		title: 'Architecture details',
		description: 'Model caches, threading and adding a vocoder.',
	},
];

rmSync(outputDirectory, { recursive: true, force: true });
mkdirSync(outputDirectory, { recursive: true });
mkdirSync(assetsDirectory, { recursive: true });

cpSync(join(siteRoot, 'content/index.md'), join(outputDirectory, 'index.md'));
cpSync(join(siteRoot, 'content/404.md'), join(outputDirectory, '404.md'));
cpSync(join(repoRoot, 'assets/icon.png'), join(assetsDirectory, 'logo.png'));

for (const page of pages) {
	let text = readFileSync(join(sourceDirectory, page.source), 'utf8');
	const heading = text.match(/^#\s+([^\n]+)\n/);
	if (heading) text = text.slice(heading[0].length);
	text = text.replace(/^\s*\n/, '');
	const frontmatter = `---\ntitle: ${JSON.stringify(page.title)}\ndescription: ${JSON.stringify(page.description)}\n---\n\n`;
	writeFileSync(join(outputDirectory, page.target), frontmatter + text);
}

console.log(`synced ${pages.length} documentation pages and the landing page`);
