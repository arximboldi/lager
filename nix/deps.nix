{ nixpkgs ? <nixpkgs>}:

with import nixpkgs {};

rec {
  libhttpserver = stdenv.mkDerivation rec {
    name = "libhttpserver-${version}";
    version = "git-${commit}";
    commit = "4895f43ed29195af70beb47bcfd1ef3ab4555665";
    src = fetchFromGitHub {
      owner = "etr";
      repo = "libhttpserver";
      rev = commit;
      sha256 = "1qg5frqvfhb8bpfiz9wivwjz2icy3si112grv188kgypws58n832";
    };
    propagatedBuildInputs = [ libmicrohttpd ];
    nativeBuildInputs = [ autoreconfHook gcc5 ];
    configureScript = "../configure";
    configurePhase = ''
      substituteInPlace configure --replace "/bin/pwd" "${coreutils}/bin/pwd"
      mkdir build && cd build
      ../configure -prefix $out
    '';
    meta = with stdenv.lib; {
      homepage = "https://github.com/etr/libhttpserver";
      description = "C++ library for creating an embedded Rest HTTP server (and more)";
      license = licenses.lgpl2;
    };
  };

  cereal = stdenv.mkDerivation rec {
    name = "cereal-${version}";
    version = "git-arximboldi-${commit}";
    commit = "f158a44a3277ec2e1807618e63bcb8e1bd559649";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "cereal";
      rev = commit;
      sha256 = "1zny1k00npz3vrx6bhhdd2gpsy007zjykvmf5af3b3vmvip5p9sm";
    };
    nativeBuildInputs = [ cmake ];
    cmakeFlags="-DJUST_INSTALL_CEREAL=true";
    meta = with stdenv.lib; {
      homepage = "http://uscilab.github.io/cereal";
      description = "A C++11 library for serialization";
      license = licenses.bsd3;
    };
  };

  immer = stdenv.mkDerivation rec {
    name = "immer-${version}";
    version = "git-${commit}";
    commit = "ffbc180da6463f8f06af0e96336f161256422b1f";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "immer";
      rev = commit;
      sha256 = "0f05mhm2sjmvwy6ix4gfahig26rx63c1ji2zr8nvxy75gslnfkpn";
    };
    nativeBuildInputs = [ cmake ];
    meta = with stdenv.lib; {
      homepage = "http://sinusoid.es/immer";
      description = "Immutable data structures for C++";
      license = licenses.lgpl3;
    };
  };

  zug = stdenv.mkDerivation rec {
    name = "zug-${version}";
    version = "git-${commit}";
    commit = "5b72389901844ddaca1fb63e44d41f77a6165c2a";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "zug";
      rev = commit;
      sha256 = "1r62khc0fn5a10h138i2jvmf9qlbs1q7kqxvx380ilcqhl32z2jw";
    };
    nativeBuildInputs = [ cmake ];
    meta = with stdenv.lib; {
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
    meta = with stdenv.lib; {
      description = "Immediate mode UI library";
      license = licenses.lgpl3;
    };
  };
}
