<!-- Thanks for contributing. Please fill in every section: CONTRIBUTING.md says what each needs.
CI marks a pull request whose "Tests" or "How I tested it" section is empty as not ready. -->

## What this changes and why

<!-- One or two sentences. Link the issue it fixes: "Fixes #123". -->

## Tests

<!-- The suites you ran, and the tests you added or changed. If no test fits, say why
(e.g. "drawing only; see the screenshots"). The suites: firmware/test/{i18n,ics,ebook,tutor7}/run.sh,
firmware/test/ebook/fonts.sh, and `node test/run.mjs` in site/. -->

## How I tested it

<!-- The steps you took on a real board (or in the setup app or on the site), with screenshots or photos:
before and after if a screen changed, portrait and landscape if it draws differently.
`firmware/tools/board.py shot out.png` gives the screen as drawn; run `demo on` first. -->

## Size and memory

<!-- For firmware changes: "Global variables use N bytes" and the sketch size from `./upload.sh build`,
before and after. Write "not firmware" otherwise. -->

## Checklist

- [ ] It builds (`./upload.sh build`) and the test suites pass
- [ ] New or changed behaviour has a test, or I said above why it can't
- [ ] I tried it on a board, in portrait and landscape if the screen changed, and attached screenshots
- [ ] New text goes through `S()` in all ten languages (`firmware/test/i18n/run.sh` passes)
- [ ] Big buffers and task stacks are in PSRAM; the internal RAM change is reported above
- [ ] README or docs/guide.md updated if something a user sees or does changed
- [ ] No WiFi passwords, keys, tokens, names, addresses or other private values in the diff or the screenshots
- [ ] One change per pull request, with commit messages that say what changed and why
