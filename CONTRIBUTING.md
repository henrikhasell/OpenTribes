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
- Keep the marker to one line: the verb, then five words at most.

## Merge requests

- Title: the commit subject.
- Description: a line of context, then the same bullets, then the reasoning at
  whatever length it needs. This is the one place it goes.
- One concern per branch. Keep history linear: rebase onto `opentribes` rather
  than merging it in.
- No AI attribution or generated-by trailers, in commits or descriptions.
