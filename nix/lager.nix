{
  pkgs,
  lib,
  stdenv,
  cmake,
  ninja,
  pkg-config,
  boost,
  catch2,
  ncurses,
  SDL2,
  SDL2_ttf,
  immer,
  zug,
  sources ? ../.,
  withTests ? false,
  withExamples ? false,
  withDebug ? false,
}:

let
  cereal = pkgs.callPackage ./cereal.nix { };
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

  buildInputs =
    [
      boost
      cereal
      immer
      zug
    ]
    ++ lib.optionals withTests [
      catch2
    ]
    ++ lib.optionals withExamples [
      ncurses
      SDL2
      SDL2_ttf
    ];

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
  ];

  doCheck = withTests;

  cmakeBuildType = if withDebug then "Debug" else "Release";

  cmakeFlags = [
    (lib.cmakeBool "lager_BUILD_TESTS" withTests)
    (lib.cmakeBool "lager_BUILD_EXAMPLES" withExamples)
    (lib.cmakeBool "lager_BUILD_DEBUGGER_EXAMPLES" false)
    (lib.cmakeBool "lager_BUILD_DOCS" false)
  ];

  ninjaFlags =
    lib.optionals withTests [ "tests" ]
    ++ lib.optionals withExamples [ "examples" ];

  meta = {
    homepage = "https://github.com/arximboldi/lager";
    description = "C++ library for value-oriented design using the unidirectional data-flow architecture";
    license = lib.licenses.mit;
  };
}
