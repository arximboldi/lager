{
  flakeInputs ? (import ./nix/flake-compat.nix { }).inputs,
  pkgs ? import flakeInputs.nixpkgs { },
  toolchain ? "",
  immer ? (import flakeInputs.immer { inherit pkgs; }).packages.${pkgs.system}.default,
  zug ? (import flakeInputs.zug { inherit pkgs; }).packages.${pkgs.system}.default,
}:

let
  lib = pkgs.lib;
  theStdenv = import ./nix/choose-stdenv.nix { inherit pkgs toolchain; };
  qt = pkgs.qt5;
  qtver = qt.qtbase.version;
  imgui = pkgs.callPackage ./nix/imgui.nix { };
  lager = pkgs.callPackage ./nix/lager.nix {
    stdenv = theStdenv;
    inherit immer zug;
    withTests = true;
    withExamples = true;
  };
in
theStdenv.mkDerivation {
  name = "lager-dev";

  buildInputs =
    lager.buildInputs
    ++ [
      pkgs.catch2
      pkgs.ccache
      pkgs.ncurses
      pkgs.SDL2
      pkgs.SDL2_ttf
      pkgs.glibcLocales
      imgui
      qt.qtbase
      qt.qtdeclarative
      qt.qtquickcontrols
      qt.qtquickcontrols2
      qt.qtgraphicaleffects
      (pkgs.python3.withPackages (ps: [
        ps.click
        ps.requests
      ]))
    ]
    ++ lib.optionals theStdenv.cc.isClang [ pkgs.lldb ]
    ++ lib.optionals theStdenv.cc.isGNU [ pkgs.gdb ];

  nativeBuildInputs = lager.nativeBuildInputs;

  hardeningDisable = [ "fortify" ];

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
    export IMGUI_SOURCE_DIR=${imgui}
    export EM_CACHE=`mktemp -d`
  '';
}
