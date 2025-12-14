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
  devcontainer.settings.containerEnv = {
    COPILOT_GITHUB_TOKEN = "$" + "{localEnv:COPILOT_GITHUB_TOKEN}";
    GOOGLE_GENAI_USE_GCA = "$" + "{localEnv:GOOGLE_GENAI_USE_GCA}";
    GOOGLE_CLOUD_ACCESS_TOKEN = "$" + "{localEnv:GOOGLE_CLOUD_ACCESS_TOKEN}";
    GIT_AUTHOR_NAME = "$" + "{localEnv:GIT_AUTHOR_NAME}";
    GIT_AUTHOR_EMAIL = "$" + "{localEnv:GIT_AUTHOR_EMAIL}";
  };
}
