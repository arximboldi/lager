{ nixpkgs ? <nixpkgs>}:

with import nixpkgs {};

rec {
  cereal = stdenv.mkDerivation rec {
    name = "cereal-${version}";
    version = "git-arximboldi-${commit}";
    commit = "4bfaf5fee1cbc69db4614169092368a29c7607c4";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "cereal";
      rev = commit;
      sha256 = "17gwhwhih4737wzm24c45y3ch69jzw2mi8prj1pdlxff8f1pki8v";
    };
    nativeBuildInputs = [ cmake ];
    cmakeFlags = [
      "-DJUST_INSTALL_CEREAL=true"
    ];
    meta = with lib; {
      homepage = "http://uscilab.github.io/cereal";
      description = "A C++11 library for serialization";
      license = licenses.bsd3;
    };
  };

  immer = stdenv.mkDerivation rec {
    name = "immer-${version}";
    version = "git-${commit}";
    commit = "df6ef46d97e1fe81f397015b9aeb32505cef653b";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "immer";
      rev = commit;
      sha256 = "sha256-fV6Rqbg/vtUH2DdgLYULl0zLM3WUSG1qYLZtqAhaWQw=";
    };
    dontUseCmakeBuildDir = true;
    nativeBuildInputs = [ cmake ];
    cmakeFlags = [
      "-Dimmer_BUILD_TESTS=OFF"
      "-Dimmer_BUILD_EXAMPLES=OFF"
    ];
    meta = with lib; {
      homepage = "http://sinusoid.es/immer";
      description = "Immutable data structures for C++";
      license = licenses.lgpl3;
    };
  };

  zug = stdenv.mkDerivation rec {
    name = "zug-${version}";
    version = "git-${commit}";
    commit = "7c22cc138e2a9a61620986d1a7e1e9730123f22b";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "zug";
      rev = commit;
      sha256 = "sha256-/0HnSUmmyX49L6pJk9QlviFF2FYi5o+x++94wwYwWjk=";
    };
    nativeBuildInputs = [ cmake ];
    dontUseCmakeBuildDir = true;
    cmakeFlags = [
      "-Dzug_BUILD_TESTS=OFF"
      "-Dzug_BUILD_EXAMPLES=OFF"
    ];
    meta = with lib; {
      homepage = "http://sinusoid.es/zug";
      description = "Transducers for C++";
    };
  };

  imgui = stdenv.mkDerivation rec {
    name = "imgui-${version}";
    version = "git-${commit}";
    commit = "6ffee0e75e8f677c5fd8280dfe544c3fcb325f45";
    src = fetchFromGitHub {
      owner = "ocornut";
      repo = "imgui";
      rev = commit;
      sha256 = "0z84phn3d71gsawmynxj1l32fxq73706z65iqp1sx7i1qpnyz43a";
    };
    buildPhase = "";
    installPhase = ''
      mkdir $out
      cp $src/*.h $out/
      cp $src/*.cpp $out/
      cp $src/examples/imgui_impl_* $out/
    '';
    meta = with lib; {
      description = "Immediate mode UI library";
      license = licenses.lgpl3;
    };
  };
}
