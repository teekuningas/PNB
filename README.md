PNB - Not Baseball Since 2013
===

## Installation

Download and extract the zip file from the Releases.

## Prerequisites (NixOS)

```nix
virtualisation.podman.enable = true;
virtualisation.podman.dockerCompat = true;
virtualisation.containers.containersConf.settings.network.default_rootless_network_cmd = "slirp4netns";
systemd.user.services.podman.path = [ "/run/wrappers/" ];

nixpkgs.overlays = [(final: prev: {
  devcontainer = prev.devcontainer.overrideAttrs (old: {
    postInstall = ''
      makeWrapper "${prev.nodejs_20}/bin/node" "$out/bin/devcontainer" \
        --add-flags "$out/libexec/devcontainer.js" \
        --prefix PATH : ${prev.lib.makeBinPath [prev.podman]} \
        --set DEVCONTAINER_DOCKER_PATH "${prev.podman}/bin/podman"
    '';
  });
})];

environment.systemPackages = with pkgs; [ podman slirp4netns devcontainer ];
```

## Usage

```bash
devenv shell
export COPILOT_GITHUB_TOKEN=$(gh auth token)
export GIT_AUTHOR_NAME="$(git config user.name)"
export GIT_AUTHOR_EMAIL="$(git config user.email)"
devcontainer build --workspace-folder .
devcontainer up --workspace-folder .
devcontainer exec --workspace-folder . make watch_task_agent_copilot
```
