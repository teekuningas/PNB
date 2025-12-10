PNB - Not Baseball Since 2013
===

## Installation

Download and extract the zip file from the Releases.

## Agent

```bash
devenv shell
export COPILOT_GITHUB_TOKEN=$(gh auth token)
export GIT_AUTHOR_NAME="$(git config user.name)"
export GIT_AUTHOR_EMAIL="$(git config user.email)"
devcontainer build --workspace-folder .
devcontainer up --workspace-folder .
devcontainer exec --workspace-folder . make watch_task_agent
```
