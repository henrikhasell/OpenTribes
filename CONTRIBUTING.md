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

## Marking the engine

Mark every stock Torque file you have been at, so what is ours can be told from
what is GarageGames'. Sign it with your own initials or name, then the verb for
what you did to the thing below.

```
// HH: Added for Tribes 2 compatibility.
// HH: Modified for Tribes 2 compatibility.
```

- `Added` — it is new: a function, a console binding, a member, a file Torque
  3D does not have.
- `Modified` — it already existed and you changed it. A statement put inside a
  stock function modifies that function.
- The test is whether the function exists upstream. Nothing back means ours:

```
git show upstream/master:Engine/source/gui/core/guiCanvas.cpp | grep createCanvas
```

- A file we wrote entirely has nothing to modify, so its markers read `Added`.
- Keep the marker to the verb and a line of why.

## Merge requests

- Title: the commit subject.
- Description: a line of context, then the same bullets.
- One concern per branch. Keep history linear: rebase onto `opentribes` rather
  than merging it in.
- No AI attribution or generated-by trailers, in commits or descriptions.
