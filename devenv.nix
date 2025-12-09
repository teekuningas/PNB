{ pkgs, lib, config, inputs, ... }:
{
  languages.python.enable = true;

  
  packages = with pkgs; [
    glfw3
    glew
    xorg.libX11
    libGL
    libGLU
    miniaudio # miniaudio is not in flake.nix, but minixml is
    minixml # miniaudio is not in flake.nix, but minixml is
    gcc
    astyle
    alsa-lib
    gh # Add GitHub CLI
    bashInteractive # Add interactive bash for robust shell environment
    github-copilot-cli # User mentioned this in their snippet
  ];

  env.LD_LIBRARY_PATH = lib.makeLibraryPath (with pkgs; [
    glfw3
    glew
    xorg.libX11
    libGL
    libGLU
    minixml
    alsa-lib
  ]);

  devcontainer.enable = true;
  devcontainer.settings.remoteUser = "root";
  devcontainer.settings.updateContentCommand = "devenv print-dev-env >> /etc/bash.bashrc";
  devcontainer.settings.containerEnv.COPILOT_GITHUB_TOKEN = "$" + "{localEnv:COPILOT_GITHUB_TOKEN}";
}
