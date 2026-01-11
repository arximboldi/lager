{
  description = "C++ library for value-oriented design using the unidirectional data-flow architecture";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
    nix-github-actions.url = "github:nix-community/nix-github-actions";
    nix-github-actions.inputs.nixpkgs.follows = "nixpkgs";
    gitignore.url = "github:hercules-ci/gitignore.nix";
    gitignore.inputs.nixpkgs.follows = "nixpkgs";
    immer.url = "github:arximboldi/immer";
    immer.inputs.nixpkgs.follows = "nixpkgs";
    zug.url = "github:arximboldi/zug";
    zug.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      nix-github-actions,
      gitignore,
      immer,
      zug,
    }:
    {
      githubActions = nix-github-actions.lib.mkGithubMatrix {
        checks = nixpkgs.lib.getAttrs [
          "x86_64-linux"
          "aarch64-linux"
          "aarch64-darwin"
        ] self.checks;
      };
    }
    // flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lib = nixpkgs.lib;
        toolchains = [
          "gnu"
          "llvm"
        ];
      in
      {
        devShells = (
          with self.devShells.${system};
          {
            default = pkgs.callPackage ./shell.nix {
              inherit pkgs;
              immer = immer.packages.${system}.default;
              zug = zug.packages.${system}.default;
            };
          }
          // lib.attrsets.genAttrs toolchains (toolchain: default.override { inherit toolchain; })
        );

        checks = self.packages.${system};

        packages = (
          with self.packages.${system};
          {
            default = lager;

            lager = pkgs.callPackage nix/lager.nix {
              sources = ./.;
              immer = immer.packages.${system}.default;
              zug = zug.packages.${system}.default;
            };

            tests = lager.override {
              withTests = true;
            };

            tests-debug = tests.override {
              withDebug = true;
            };

            examples = lager.override {
              withExamples = true;
            };
          }
        );
      }
    );
}
