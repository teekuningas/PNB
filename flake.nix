{
  description = "PNB";
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/23.05";
  };
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        formatter = pkgs.nixfmt;
        devShells.default = pkgs.mkShell {
          buildInputs =
            with pkgs; [
              glfw2
              glew
              xorg.libX11
              libGL
              libGLU
              gcc
              astyle
            ];
        };
      }
    );
}

