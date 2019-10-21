{ compiler ? "",
  nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "5ac6ab091a4883385e68571425fb7fef4d74c207";
    sha256 = "0rksyhnnj5028n2ql3jkf98vpd8cs1qf6rckgvx9jq2zf1xqsbla";
  }}:

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

in
theStdenv.mkDerivation rec {
  name = "lager-env";
  buildInputs = [
    gcc7
    cmake
    ccache
    ncurses
    boost
    glibcLocales
    deps.libhttpserver
    deps.cereal
    deps.immer
    deps.zug
    sass
    SDL2
    SDL2_ttf
    qt5.qtbase
    qt5.qtdeclarative
    qt5.qtquickcontrols
    qt5.qtquickcontrols2
    qt5.qtgraphicaleffects
    elmPackages.elm-reactor
    elmPackages.elm-make
    elmPackages.elm-package
    (python3.withPackages (pkgs: with pkgs; [
      click
      requests
    ]))
    old-nixpkgs.doxygen
    (old-nixpkgs.python.withPackages (ps: [
      ps.sphinx
      docs.breathe
      docs.recommonmark
    ]))
  ];
  shellHook = ''
    export LAGER_ROOT=`dirname ${toString ./shell.nix}`
    export LAGER_RESOURCES_PATH="$LAGER_ROOT"/resources
    addToSearchPath PATH "$LAGER_ROOT/build"
    addToSearchPath PATH "$LAGER_ROOT/build/example"
    addToSearchPath PATH "$LAGER_ROOT/build/test"
    addToSearchPath QML2_IMPORT_PATH ${qt5.qtdeclarative.bin}/lib/qt-5.11/qml
    addToSearchPath QML2_IMPORT_PATH ${qt5.qtquickcontrols}/lib/qt-5.11/qml
    addToSearchPath QML2_IMPORT_PATH ${qt5.qtquickcontrols2.bin}/lib/qt-5.11/qml
    addToSearchPath QML2_IMPORT_PATH ${qt5.qtgraphicaleffects}/lib/qt-5.11/qml
    addToSearchPath QT_PLUGIN_PATH ${qt5.qtsvg.bin}/lib/qt-5.11/plugins
    export QT_QPA_PLATFORM_PLUGIN_PATH=${qt5.qtbase}/lib/qt-5.11/plugins
  '';
}
