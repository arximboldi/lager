{
  pkgs,
  lib,
  stdenv,
  cmake,
  ninja,
  gnumake,
  pkg-config,
  boost,
  catch2,
  ncurses,
  glibcLocales,
  SDL2,
  SDL2_ttf,
  sass,
  immer,
  zug,
  sources ? ../.,
  withTests ? false,
  withExamples ? false,
  withDebug ? false,
  withDocs ? false,
}:

let
  docs = pkgs.callPackage ./docs.nix { };
  cereal = pkgs.callPackage ./cereal.nix { };
  imgui = pkgs.callPackage ./imgui.nix { };

  qt = pkgs.qt5;

in
stdenv.mkDerivation {
  name = builtins.concatStringsSep "-" (
    [ "lager" ]
    ++ lib.optionals withDebug [ "debug" ]
    ++ lib.optionals withTests [ "tests" ]
    ++ lib.optionals withExamples [ "examples" ]
  );
  version = "git";

  src = sources;

  buildInputs = [
    boost
    cereal
    immer
    zug
  ]
  ++ lib.optionals withTests [
    catch2
    qt.qtbase
    qt.qtdeclarative
  ]
  ++ lib.optionals withExamples [
    ncurses
    glibcLocales
    SDL2
    SDL2_ttf
    imgui
    qt.qtbase
    qt.qtdeclarative
    qt.qtquickcontrols
    qt.qtquickcontrols2
    qt.qtgraphicaleffects
    sass
  ]
  ++ lib.optionals (withExamples && stdenv.hostPlatform.system == "x86_64-linux") [
    # stick to old versions that worked when we tested this...  seem
    # to only work on linux x86_64 for now, but it's fine we don't
    # need this for much
    docs.pkgs.elmPackages.elm-reactor
    docs.pkgs.elmPackages.elm-make
    docs.pkgs.elmPackages.elm-package
  ]
  ++ lib.optionals withDocs [
    docs.doxygen
    docs.python
  ];

  nativeBuildInputs = [
    cmake
    ninja
    gnumake
    pkg-config
  ];

  # on macos "build" conflicts with the BUILD file
  cmakeBuildDir = "project";

  doCheck = withTests;

  dontWrapQtApps = true;

  cmakeBuildType = if withDebug then "Debug" else "Release";

  cmakeFlags = [
    (lib.cmakeBool "lager_BUILD_TESTS" withTests)
    (lib.cmakeBool "lager_BUILD_EXAMPLES" withExamples)
    (lib.cmakeBool "lager_BUILD_DEBUGGER_EXAMPLES" withExamples)
    (lib.cmakeBool "lager_BUILD_DOCS" withDocs)
  ];

  ninjaFlags = lib.optionals withTests [ "tests" ] ++ lib.optionals withExamples [ "examples" ];

  meta = {
    homepage = "https://github.com/arximboldi/lager";
    description = "C++ library for value-oriented design using the unidirectional data-flow architecture";
    license = lib.licenses.mit;
  };
}
