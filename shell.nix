{
  compiler ? "",
  rev      ? "f904e3562aabca382d12f8471ca2330b3f82899a",
  sha256   ? "1lsa3sjwp1v3nv2jjpkl5lf9dncplwihmavasalg9fq1217pmzmb",
  nixpkgs  ? builtins.fetchTarball {
    name   = "nixpkgs-${rev}";
    url    = "https://github.com/nixos/nixpkgs/archive/${rev}.tar.gz";
    sha256 = sha256;
  },
}:

with import nixpkgs {};

let
  # For the documentation tools we use an older Nixpkgs since the
  # newer versions seem to be not working great...
  old-nixpkgs-src = fetchFromGitHub {
                      owner  = "NixOS";
                      repo   = "nixpkgs";
                      rev    = "d0d905668c010b65795b57afdf7f0360aac6245b";
                      sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
                    };
  old-nixpkgs     = import old-nixpkgs-src {};
  docs            = import ./nix/docs.nix { nixpkgs = old-nixpkgs-src; };
  deps            = import ./nix/deps.nix { inherit nixpkgs; };
  compilerPkg   = if compiler != ""
                  then pkgs.${compiler}
                  else stdenv.cc;
  theStdenv     = if compilerPkg.isClang
                  then clangStdenv
                  else stdenv;
  qt            = qt5;
  qtver         = qt.qtbase.version;

in
theStdenv.mkDerivation rec {
  name = "lager-env";
  buildInputs = [
    catch2
    cmake
    ccache
    ncurses
    boost
    pkgconfig
    libiconvReal
    glibcLocales
    deps.cereal
    deps.immer
    deps.zug
    deps.imgui
    SDL2
    SDL2_ttf
    qt.qtbase
    qt.qtdeclarative
    qt.qtquickcontrols
    qt.qtquickcontrols2
    qt.qtgraphicaleffects
    (python3.withPackages (pkgs: with pkgs; [
      click
      requests
    ]))
  ] ++ lib.optionals stdenv.isLinux [
    emscripten
    old-nixpkgs.doxygen
    (old-nixpkgs.python.withPackages (ps: [
      ps.sphinx
      docs.breathe
      docs.recommonmark
      ]))
    old-nixpkgs.sass
    old-nixpkgs.elmPackages.elm-reactor
    old-nixpkgs.elmPackages.elm-make
    old-nixpkgs.elmPackages.elm-package
  ];
  shellHook = ''
    export LAGER_ROOT=`dirname ${toString ./shell.nix}`
    export LAGER_RESOURCES_PATH="$LAGER_ROOT"/resources
    addToSearchPath PATH "$LAGER_ROOT/build"
    addToSearchPath PATH "$LAGER_ROOT/build/example"
    addToSearchPath PATH "$LAGER_ROOT/build/test"
    addToSearchPath QML2_IMPORT_PATH ${qt.qtquickcontrols}/lib/qt-${qtver}/qml
    addToSearchPath QML2_IMPORT_PATH ${qt.qtquickcontrols2.bin}/lib/qt-${qtver}/qml
    addToSearchPath QML2_IMPORT_PATH ${qt.qtgraphicaleffects}/lib/qt-${qtver}/qml
    addToSearchPath QT_PLUGIN_PATH ${qt.qtsvg.bin}/lib/qt-${qtver}/plugins
    export QT_QPA_PLATFORM_PLUGIN_PATH=${qt.qtbase}/lib/qt-${qtver}/plugins
    export IMGUI_SOURCE_DIR=${deps.imgui}
    export EM_CACHE=`mktemp -d`
  '';
}
