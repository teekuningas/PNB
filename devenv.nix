{ pkgs, lib, config, inputs, ... }:
{
  languages.python.enable = true;

  packages = with pkgs; [
    glfw3
    glew
    xorg.libX11
    libGL
    libGLU
    miniaudio
    minixml
    gcc
    astyle
    alsa-lib
    gh
    bashInteractive
    github-copilot-cli
    gemini-cli
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
