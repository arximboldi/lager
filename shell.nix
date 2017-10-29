{ compiler ? "gcc7",
  nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "d0d905668c010b65795b57afdf7f0360aac6245b";
    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
  }}:

with import nixpkgs {};

let
  compiler_pkg = pkgs.${compiler};
  native_compiler = compiler_pkg.isClang == stdenv.cc.isClang;
  deps = import ./nix/deps.nix { inherit nixpkgs; };
in

stdenv.mkDerivation rec {
  name = "ewig-env";
  buildInputs = [
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
  ] ++ stdenv.lib.optionals compiler_pkg.isClang [libcxx libcxxabi];
  propagatedBuildInputs = stdenv.lib.optional (!native_compiler) compiler_pkg;
  nativeBuildInputs = stdenv.lib.optional native_compiler compiler_pkg;
}
