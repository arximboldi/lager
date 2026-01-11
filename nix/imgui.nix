{
  lib,
  stdenv,
  fetchFromGitHub,
}:

stdenv.mkDerivation rec {
  pname = "imgui";
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

  meta = {
    homepage = "https://github.com/ocornut/imgui";
    description = "Immediate mode UI library";
    license = lib.licenses.mit;
  };
}
