// Draws the mermaid diagrams on the page. The library itself is loaded by a
// blocking script in front of the first diagram (see src/plugins/remark-docs.mjs).
async function drawMermaid() {
	const nodes = [...document.querySelectorAll('pre.mermaid')];
	if (!nodes.length) return;
	const mermaid = window.mermaid;
	if (!mermaid) return;
	const dark = document.documentElement.dataset.theme === 'dark';
	mermaid.initialize({
		startOnLoad: false,
		securityLevel: 'strict',
		theme: dark ? 'dark' : 'default',
	});
	await mermaid.run({ nodes });
}

drawMermaid().catch((error) => console.error('mermaid', error));
