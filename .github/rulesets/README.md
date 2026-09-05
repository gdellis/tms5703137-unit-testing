# Branch rulesets

`main-branch-protection.json` is the protection for the default branch, kept here so
it is reviewable and re-appliable rather than existing only as settings someone
clicked once. GitHub does not read it from the repository — you have to apply it.

## Apply it

From a clone, with the [`gh` CLI](https://cli.github.com/) authenticated as someone
with admin on the repository (`gh` fills in `{owner}` and `{repo}`):

```sh
gh api --method POST repos/{owner}/{repo}/rulesets \
    --input .github/rulesets/main-branch-protection.json
```

Or in the browser: **Settings → Rules → Rulesets → New ruleset → Import a ruleset**,
then upload the file.

To change it later, edit the file, then `PUT` it over the existing ruleset (get the id
from `gh api repos/{owner}/{repo}/rulesets`):

```sh
gh api --method PUT repos/{owner}/{repo}/rulesets/<id> \
    --input .github/rulesets/main-branch-protection.json
```

## What it does

Targets `~DEFAULT_BRANCH`, so it follows the default branch instead of hard-coding
`main`.

| Rule | Effect |
|---|---|
| `deletion` | `main` cannot be deleted |
| `non_fast_forward` | no force-pushes to `main` |
| `pull_request` | changes reach `main` only through a pull request, and review threads must be resolved before merging |
| `required_status_checks` | all six CI jobs must pass on the merge commit |

The six required checks are the job *names* from `.github/workflows/ci.yml`. **If you
rename a job, rename it here too** — a required check that never reports blocks every
merge, and a job that is no longer required stops gating silently.

## Choices worth knowing about

- **Zero required approvals.** GitHub does not let you approve your own pull request,
  so on a repository with one active maintainer any non-zero count makes every PR
  unmergeable without a bypass. Raise `required_approving_review_count` once more than
  one person is reviewing.
- **No bypass actors.** Nobody, including repository admins, can push straight to
  `main`. If you want an escape hatch, add *Repository admin* under **Bypass list** in
  the ruleset UI — that is easier to get right than the numeric role id in JSON.
- **`strict_required_status_checks_policy` is false**, so a branch does not have to be
  rebased onto the newest `main` before merging. Set it to `true` if you want CI to
  have run against the exact merge result, at the cost of re-running it whenever `main`
  moves.
- **Merge commits are not restricted.** The repository's history uses them; add
  `allowed_merge_methods` to the `pull_request` rule if you ever want to force squash
  or rebase.
- **`enforcement` is `active`.** Set it to `disabled` to park the ruleset without
  deleting it. (`evaluate`, which reports violations without blocking, needs an
  organization on GitHub Enterprise.)
