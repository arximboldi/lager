{ nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "5ac6ab091a4883385e68571425fb7fef4d74c207";
    sha256 = "0rksyhnnj5028n2ql3jkf98vpd8cs1qf6rckgvx9jq2zf1xqsbla";
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
    glibcLocales
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
