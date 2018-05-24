{ nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "d45e3e36f85ebf98f7e9bcb4105c44463a171655";
    sha256 = "0k0sf2r3cngd99v18l4panmxq2l5bqw85cvg4wcm4affw61w037a";
  }}:

with import nixpkgs {};

let
  deps = import ./nix/deps.nix { inherit nixpkgs; };
in
stdenv.mkDerivation rec {
  name = "lager-env";
  buildInputs = [
    gcc7
    cmake
    ncurses
    boost
    deps.libhttpserver
    deps.cereal
    deps.immer
    sass
    SDL2
    SDL2_ttf
    elmPackages.elm-reactor
    elmPackages.elm-make
    elmPackages.elm-package
    (python3.withPackages (pkgs: with pkgs; [
      click
      requests
    ]))
  ];
  shellHook = ''
    export LAGER_ROOT=`dirname ${toString ./shell.nix}`
    export LAGER_RESOURCES_PATH="$LAGER_ROOT"/resources
    export PATH=$PATH:"$LAGER_ROOT/build"
    export PATH=$PATH:"$LAGER_ROOT/build/example"
    export PATH=$PATH:"$LAGER_ROOT/build/test"
  '';
}
