{
  flake ? import ./nix/flake-compat.nix { },
  pkgs ? import flake.inputs.nixpkgs { },
}:

let
  inherit (pkgs) lib;
  inherit (import flake.inputs.gitignore { inherit lib; })
    gitignoreSource
    ;

  nixFilter = name: type: !(lib.hasSuffix ".nix" name);
  srcFilter =
    src:
    lib.cleanSourceWith {
      filter = nixFilter;
      src = gitignoreSource src;
    };

  immer = (import flake.inputs.immer { inherit pkgs; }).packages.${pkgs.system}.default;
  zug = (import flake.inputs.zug { inherit pkgs; }).packages.${pkgs.system}.default;

in
pkgs.callPackage ./nix/lager.nix {
  sources = srcFilter ./.;
  inherit immer zug;
}
