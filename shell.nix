{
  inputs ? (import ./nix/flake-compat.nix { }).inputs,
  pkgs ? import inputs.nixpkgs { },
  toolchain ? "",
  immer ? (import inputs.immer { inherit pkgs; }).packages.${pkgs.system}.default,
  zug ? (import inputs.zug { inherit pkgs; }).packages.${pkgs.system}.default,
}:

let
  toolchain-stdenv = import ./nix/choose-stdenv.nix { inherit pkgs toolchain; };
  stdenv = toolchain-stdenv;
  lib = pkgs.lib;
  qt = pkgs.qt5;
  qtver = qt.qtbase.version;
  lager = pkgs.callPackage ./nix/lager.nix {
    inherit stdenv immer zug;
    withTests = true;
    withExamples = true;
    withDocs = stdenv.isLinux;
  };
  imgui = pkgs.callPackage ./nix/imgui.nix { };

in
pkgs.mkShell.override { inherit stdenv; } {
  inputsFrom = [ lager ];
  packages =
    [
      pkgs.catch2
      pkgs.ccache
      pkgs.ncurses
      (pkgs.python3.withPackages (ps: [
        ps.click
        ps.requests
      ]))
    ]
    ++ lib.optionals stdenv.cc.isClang [ pkgs.lldb ]
    ++ lib.optionals stdenv.cc.isGNU [ pkgs.gdb ];

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
