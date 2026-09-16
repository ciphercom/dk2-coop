# Co-op promotional website

The static site lives in `website/`. It uses HTML, CSS, and a small local guestbook script, with local assets and no package installation or build step. All internal assets use relative URLs so the GitHub Pages `/dk2-coop/` project path works.

## Preview

Open `website/index.html` directly, or serve the folder:

```powershell
python -m http.server 8080 --directory website
```

Then visit http://localhost:8080.

## GitHub Pages

1. In this repository's Settings > Pages, choose **GitHub Actions** as the publishing source.
2. Push the website and `.github/workflows/pages.yml` to `main`, or run the **Deploy website to GitHub Pages** workflow manually after it is pushed.
3. The expected project URL is https://ciphercom.github.io/dk2-coop/.

The workflow uploads only `website/`, not the game repository. It follows GitHub's [custom workflow guidance](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages).

## Release copy

The page intentionally describes the completed co-op release, as requested. Download links lead to this repository's release archive. Publish the matching co-op package there when the co-op code is ported; update installation details if its launcher or layout changes. Campaign progress currently describes host-side persistence.

## Retro flourishes

The scrolling dispatch strip and blinking NEW badge share a **Pause the madness** checkbox. Reduced-motion preferences disable both animations and display the full dispatch text. The visitor counter is deliberately fixed at 000666; the joke is disclosed below it. The guestbook only displays a local punchline: it sends and stores nothing, and its form stays disabled without JavaScript.

## Artwork

`website/assets/dungeon-banner.webp` is original promotional illustration generated with the built-in image-generation tool, not a gameplay screenshot or official game artwork. `stone.svg` and `favicon.svg` are code-authored site graphics. All assets are served locally.

Generation prompt:

> Use case: stylized-concept. Create a wide 3:1 illustrated banner for a 1999-style Dungeon Keeper 2 co-op fan website. No text, no logos, no interface, no lettering. Scene: two hulking red horned demons in battered dark iron armor, seen on left and right, conspiratorially leaning over a dungeon map on a rough stone table, small mischievous imp between them. Underground medieval dungeon, hot orange braziers, dark carved stone, molten red backlight, scattered gold. Humorous sinister partnership. A hand-painted late-1990s PC game box-art illustration with airbrushed shading, visibly coarse grain and dithered texture, dramatic deep shadows. Composition panoramic, demons prominent, faces and table within central horizontal band. Restrained black, oxblood red, burnt orange, tarnished gold. Finished artwork edge to edge.
