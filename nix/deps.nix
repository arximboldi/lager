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
    commit = "a1271fa712342f5c6dfad876820da17e10c28214";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "immer";
      rev = commit;
      sha256 = "1bqkinkbp1b1aprg7ydfrbfs7gi779nypwvh9fj129frq1c2rxw5";
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
    commit = "be20cae36e7e5876bf5bfb08b2a0562e1db3b546";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "zug";
      rev = commit;
      sha256 = "0vmcnspg9ys4qkj228kgvmpb5whly1cwx30sbg21x2iqs7y11ggx";
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
