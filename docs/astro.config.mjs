import { defineConfig } from 'astro/config';
import { unified } from '@astrojs/markdown-remark';
import starlight from '@astrojs/starlight';
import rehypeKatex from 'rehype-katex';
import remarkMath from 'remark-math';
import { remarkDocs } from './src/plugins/remark-docs.mjs';
import { sidebar } from './sidebar.mjs';

// GitHub Pages project site: https://yuzukitsuru.github.io/lessampler/
const site = 'https://yuzukitsuru.github.io';
const base = '/lessampler';

// Pages were grouped into sections under src/content/docs/. Keep the old
// top-level URLs working; Astro matches the source paths under `base`.
const redirects = Object.fromEntries(
	[
		['/build', '/getting-started/build'],
		['/configuration', '/getting-started/configuration'],
		['/rendering', '/guides/rendering'],
		['/neural', '/guides/neural'],
		['/quality', '/guides/quality'],
		['/pipeline', '/processing'],
		['/arguments', '/processing/arguments'],
		['/source', '/processing/source'],
		['/modeling', '/processing/modeling'],
		['/transform', '/processing/transform'],
		['/synthesis', '/processing/synthesis'],
		['/output', '/processing/output'],
		['/architecture-details', '/architecture'],
	].map(([from, to]) => [from, `${base}${to}/`]),
);

export default defineConfig({
	site,
	base,
	redirects,
	markdown: {
		processor: unified({
			remarkPlugins: [remarkMath, [remarkDocs, { base }]],
			rehypePlugins: [rehypeKatex],
		}),
	},
	integrations: [
		starlight({
			title: 'lessampler',
			description: 'lessampler is a Singing Voice Synthesizer',
			logo: {
				src: './src/assets/logo.png',
				alt: 'lessampler',
			},
			editLink: {
				baseUrl: 'https://github.com/YuzukiTsuru/lessampler/edit/master/docs/',
			},
			social: [
				{
					icon: 'github',
					label: 'GitHub',
					href: 'https://github.com/YuzukiTsuru/lessampler',
				},
			],
			sidebar,
			customCss: ['./src/styles/custom.css'],
			head: [
				{ tag: 'script', attrs: { type: 'module', src: `${base}/enhance.js` } },
			],
		}),
	],
});
