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

export default defineConfig({
	site,
	base,
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
