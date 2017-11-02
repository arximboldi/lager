{ nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "d0d905668c010b65795b57afdf7f0360aac6245b";
    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
  }}:

with import nixpkgs {};

let
  deps = import ./nix/deps.nix { inherit nixpkgs; };
in
stdenv.mkDerivation rec {
  name = "ewig-env";
  buildInputs = [
    gcc7
    cmake
    ncurses
    boost
    deps.libhttpserver
    deps.cereal
    deps.immer
    sass
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
  '';
}
