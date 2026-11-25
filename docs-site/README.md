# lessampler documentation site

Astro Starlight site for the Markdown documentation in [`../docs/`](../docs/).

## Develop

```bash
npm install
npm run dev
```

The dev server runs at `http://localhost:4321/lessampler/`.

`npm run build` writes the static site to `dist/`; `npm run preview` serves the build.

## Content

`scripts/sync-docs.mjs` runs before the dev server and the build. It copies the pages from `../docs/`, strips their leading `# Heading`, and adds the Starlight frontmatter. Edit `../docs/*.md`, not the generated files under `src/content/docs/`.

The landing page is tracked at `content/index.md`. The sidebar is in `sidebar.mjs`, the theme in `src/styles/custom.css`, and the site URL/base in `astro.config.mjs`.

## Deployment

`.github/workflows/deploy-docs.yml` builds this directory and deploys it to GitHub Pages when `docs/` or `docs-site/` changes on `master`. Set the repository's Pages source to **GitHub Actions** before the first run. The default project-site URL is `https://yuzukitsuru.github.io/lessampler/`; change `site` and `base` in `astro.config.mjs` for a custom domain.
