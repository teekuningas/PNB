{
  description = "Development environment for PNB";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            glfw3
            glew
            xorg.libX11
            libGL
            libGLU
            miniaudio
            minixml
            gcc
            clang-tools
            alsa-lib
            gh
            bashInteractive
            github-copilot-cli
            gemini-cli
          ];

          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath (with pkgs; [
            glfw3
            glew
            xorg.libX11
            libGL
            libGLU
            minixml
            alsa-lib
          ]);
        };
      }
    );
}
