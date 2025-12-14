PNB - Not Baseball Since 2013
===

## Installation

Download and extract the zip file from the Releases.

### GitHub Copilot Setup

GitHub Copilot uses a simple token-based authentication:

```bash
# Authenticate with GitHub CLI (one-time setup)
gh auth login

# The token is automatically extracted when starting the agent
```

### Gemini CLI Setup

Gemini CLI requires OAuth authentication to use your Google AI Pro subscription quota:

**One-time setup (on host machine):**

```bash
# 1. Force file-based storage for token extraction
export GEMINI_FORCE_FILE_STORAGE=true

# 2. Start Gemini CLI and login with your Google account
gemini

# 3. In the CLI, type: /auth
# 4. Select "Login with Google"
# 5. Complete authentication in your browser
# 6. Exit Gemini CLI
```

**Note:** This OAuth login gives you access to your Google AI Pro subscription quota instead of consuming GCP billing credits.

### Starting the Agent

```bash
# 1. Enter the development shell
devenv shell

# 2. Set up GitHub Copilot token
export COPILOT_GITHUB_TOKEN=$(gh auth token)

# 3. Set up Gemini CLI OAuth token
export GOOGLE_GENAI_USE_GCA=true
export GOOGLE_CLOUD_ACCESS_TOKEN=$(./.dev/scripts/extract_gemini_token.py)

# 4. Set up Git identity
export GIT_AUTHOR_NAME="$(git config user.name)"
export GIT_AUTHOR_EMAIL="$(git config user.email)"

# 5. Build and start the devcontainer
devcontainer build --workspace-folder .
devcontainer up --workspace-folder .

# 6. Run the agent (choose one)
devcontainer exec --workspace-folder . make watch_task_agent          # Gemini CLI
devcontainer exec --workspace-folder . make watch_task_agent_copilot  # GitHub Copilot
```
