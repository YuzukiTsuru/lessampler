# lessampler documentation site

Astro Starlight site for lessampler. The site project and the documentation pages both live in this directory.

## Develop

```bash
npm install
npm run dev
```

The dev server runs at `http://localhost:4321/lessampler/`.

`npm run build` writes the static site to `dist/`; `npm run preview` serves the build.

## Content

Pages are in `src/content/docs/`. The landing page is `src/content/docs/index.md`, the 404 page is `src/content/docs/404.md`, the sidebar is in `sidebar.mjs`, the theme in `src/styles/custom.css`, and the site URL/base in `astro.config.mjs`.

To add a page, create a Markdown file under `src/content/docs/` with a `title` frontmatter field and add its slug to `sidebar.mjs`. Relative links ending in `.md` are rewritten to site routes by `src/plugins/remark-docs.mjs`.

## Deployment

`.github/workflows/deploy-docs.yml` builds this directory and deploys it to GitHub Pages when `docs/` changes on `master`. Set the repository's Pages source to **GitHub Actions** before the first run. The default project-site URL is `https://yuzukitsuru.github.io/lessampler/`; change `site` and `base` in `astro.config.mjs` for a custom domain.
