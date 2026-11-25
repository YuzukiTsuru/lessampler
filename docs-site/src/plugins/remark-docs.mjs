// Rewrites relative Markdown links to the page URLs used by Starlight.
// The documentation in ../docs/ links pages as `quality.md`; after the sync
// script copies them into src/content/docs/, those links must become routes.
import path from 'node:path';
import { visit } from 'unist-util-visit';

export function remarkDocs({ base = '' } = {}) {
	return (tree, file) => {
		const filePath = String(file.path ?? '').replaceAll('\\', '/');
		const relative = filePath.split('/src/content/docs/')[1] ?? '';
		const directory = path.posix.dirname(relative);

		visit(tree, 'link', (node) => {
			if (typeof node.url !== 'string') return;
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
