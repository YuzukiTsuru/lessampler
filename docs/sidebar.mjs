export const sidebar = [
	{
		label: 'Getting started',
		collapsed: false,
		items: [
			{ label: 'Overview', slug: 'index' },
			{ label: 'Build', slug: 'build' },
			{ label: 'Configuration', slug: 'configuration' },
		],
	},
	{
		label: 'Rendering',
		collapsed: false,
		items: [
			{ label: 'Rendering a UST', slug: 'rendering' },
			{ label: 'Neural backends', slug: 'neural' },
			{ label: 'Quality evaluation', slug: 'quality' },
		],
	},
	{
		label: 'Processing',
		collapsed: false,
		items: [
			{ label: 'Overview', slug: 'pipeline' },
			{ label: '1. Arguments in', slug: 'arguments' },
			{ label: '2. Source and cache', slug: 'source' },
			{ label: '3. Modeling', slug: 'modeling' },
			{ label: '4. Pitch and time', slug: 'transform' },
			{ label: '5. Single-frame synthesis', slug: 'synthesis' },
			{ label: '6. Output', slug: 'output' },
		],
	},
	{
		label: 'Architecture',
		collapsed: false,
		items: [{ label: 'Architecture details', slug: 'architecture-details' }],
	},
	{
		label: 'Project',
		collapsed: false,
		items: [
			{ label: 'GitHub', link: 'https://github.com/YuzukiTsuru/lessampler' },
			{ label: 'Releases', link: 'https://github.com/YuzukiTsuru/lessampler/releases' },
			{ label: 'Demo', link: 'https://www.gloomyghost.com/lessampler/#/demo/' },
		],
	},
];
