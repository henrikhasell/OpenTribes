# Contributing

## Commit messages

`type(scope): description`, then a bullet list.

- Types: `feat`, `fix`, `docs`, `refactor`, `chore`.
- Scopes: `port` (SDL3 platform port), `shell` (Tribes 2 shell UI), `console`
  (Tribes 2 console), `compat` (Tribes 2 compatibility), `renderer`. Leave the
  scope off when none fits.
- Subject: imperative, lower case, no full stop, 72 characters or fewer.
- Body: one bullet per thing the commit does, six words at most per bullet.
  Bullets and nothing else — however much there is to say, no prose here.

```
feat(shell): give the shell its sliders

- Add ShellSliderCtrl from the binary
- Share gui/shll_scroll art with scrolling
- Derive the measurements from the art
```

Why it was done goes in the merge request description. Not in the commit body,
and not in a comment beside the code.

## Merge requests

- Title: the commit subject.
- Description: a line of context, then the same bullets, then the reasoning at
  whatever length it needs. This is the one place it goes.
- One concern per branch. Keep history linear: rebase onto `opentribes` rather
  than merging it in.
- No AI attribution or generated-by trailers, in commits or descriptions.
