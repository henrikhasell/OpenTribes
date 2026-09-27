# Contributing

## Commit messages

`type(scope): description`, then a bullet list.

- Types: `feat`, `fix`, `docs`, `refactor`, `chore`.
- Scopes: `port` (SDL3 platform port), `shell` (Tribes 2 shell UI), `console`
  (Tribes 2 console), `compat` (Tribes 2 compatibility), `renderer`. Leave the
  scope off when none fits.
- Subject: imperative, lower case, no full stop, 72 characters or fewer.
- Body: one bullet per thing the commit does, six words at most per bullet.

```
feat(shell): give the shell its sliders

- Add ShellSliderCtrl from the binary
- Share gui/shll_scroll art with scrolling
- Derive the measurements from the art
```

Explanation belongs here, not in a comment beside the code.

## Merge requests

- Title: the commit subject.
- Description: a line of context, then the same bullets.
- One concern per branch. Keep history linear: rebase onto `opentribes` rather
  than merging it in.
- No AI attribution or generated-by trailers, in commits or descriptions.
