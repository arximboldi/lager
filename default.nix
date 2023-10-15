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
    SDL2
    SDL2_ttf
  ];
  nativeBuildInputs = [
    cmake
    gcc7
    sass
    pkgconfig
  ];
  cmakeFlags = [
    "-Dlager_BUILD_TESTS=OFF"
    "-Dlager_BUILD_EXAMPLES=OFF"
  ];
  propagatedBuildInputs = [
    boost
    deps.cereal
    deps.immer
    deps.zug
  ];
  meta = {
    homepage    = "https://github.com/arximboldi/lager";
    description = "library for functional interactive c++ programs";
    license     = lib.licenses.mit;
  };
}
