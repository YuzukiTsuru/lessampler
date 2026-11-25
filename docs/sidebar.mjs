export const sidebar = [
	{
		label: 'Getting started',
		collapsed: false,
		items: [
			{ label: 'Overview', slug: 'index' },
			{ label: 'Build', slug: 'getting-started/build' },
			{ label: 'Configuration', slug: 'getting-started/configuration' },
		],
	},
	{
		label: 'Guides',
		collapsed: false,
		items: [
			{ label: 'Rendering a UST', slug: 'guides/rendering' },
			{ label: 'Neural backends', slug: 'guides/neural' },
			{ label: 'Quality evaluation', slug: 'guides/quality' },
		],
	},
	{
		label: 'Processing',
		collapsed: false,
		items: [
			{ label: 'Overview', slug: 'processing' },
			{ label: '1. Arguments in', slug: 'processing/arguments' },
			{ label: '2. Source and cache', slug: 'processing/source' },
			{ label: '3. Modeling', slug: 'processing/modeling' },
			{ label: '4. Pitch and time', slug: 'processing/transform' },
			{ label: '5. Single-frame synthesis', slug: 'processing/synthesis' },
			{ label: '6. Output', slug: 'processing/output' },
		],
	},
	{
		label: 'Architecture',
		collapsed: false,
		items: [{ label: 'Architecture details', slug: 'architecture' }],
	},
	{
		label: 'Project',
		collapsed: false,
		items: [
			{ label: 'GitHub', link: 'https://github.com/YuzukiTsuru/lessampler' },
			{ label: 'Releases', link: 'https://github.com/YuzukiTsuru/lessampler/releases' },
		],
	},
];
