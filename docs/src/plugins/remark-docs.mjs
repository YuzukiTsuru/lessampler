// Rewrites relative Markdown links to the page URLs used by Starlight and
// turns ```mermaid fences into <pre class="mermaid"> blocks. The first diagram
// on a page loads the site's own copy of mermaid from public/.
import path from 'node:path';
import { visit } from 'unist-util-visit';

const escapeHtml = (value) =>
	value.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

export function remarkDocs({ base = '' } = {}) {
	return (tree, file) => {
		const filePath = String(file.path ?? '').replaceAll('\\', '/');
		const relative = filePath.split('/src/content/docs/')[1] ?? '';
		const directory = path.posix.dirname(relative);

		let mermaidLoaded = false;
		visit(tree, (node, index, parent) => {
			if (node.type === 'code' && node.lang === 'mermaid' && parent && index !== undefined) {
				const loader = mermaidLoaded ? '' : `<script src="${base}/mermaid.min.js"></script>`;
				mermaidLoaded = true;
				parent.children[index] = {
					type: 'html',
					value: `${loader}<pre class="mermaid">${escapeHtml(node.value)}</pre>`,
				};
				return;
			}

			if (node.type !== 'link' || typeof node.url !== 'string') return;
			if (/^([a-z][a-z0-9+.-]*:|#|\/\/)/i.test(node.url)) return;

			const hashAt = node.url.indexOf('#');
			const targetPath = hashAt < 0 ? node.url : node.url.slice(0, hashAt);
			const hash = hashAt < 0 ? '' : node.url.slice(hashAt);
			if (!/\.md$/i.test(targetPath)) return;

			let target = targetPath.startsWith('/')
				? targetPath.slice(1)
				: path.posix.normalize(path.posix.join(directory, targetPath));
			target = target.replace(/\.md$/i, '').replace(/(^|\/)(README|index)$/, '');
			node.url = `${base}/${target}${target ? '/' : ''}${hash}`;
		});
	};
}
