with import <nixpkgs> {};

let
  deps = import ./nix/deps.nix {};
in
stdenv.mkDerivation rec {
  name = "lager-git";
  version = "git";
  src = builtins.filterSource (path: type:
            baseNameOf path != ".git" &&
            baseNameOf path != "build" &&
            baseNameOf path != "_build" &&
            baseNameOf path != "reports" &&
            baseNameOf path != "tools")
            ./.;
  buildInputs = [
    ncurses
  ];
  nativeBuildInputs = [
    cmake
    gcc7
    sass
    elmPackages.elm-reactor
    elmPackages.elm-make
    elmPackages.elm-package
  ];
  propagatedBuildInputs = [
    boost
    deps.libhttpserver
    deps.cereal
    deps.immer
  ];
  meta = with stdenv.lib; {
    homepage    = "https://github.com/arximboldi/lager";
    description = "library for functional interactive c++ programs";
    license     = licenses.lgpl3Plus;
  };
}
