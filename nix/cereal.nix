{
  lib,
  stdenv,
  cmake,
  fetchFromGitHub,
}:

stdenv.mkDerivation rec {
  pname = "cereal";
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

  meta = {
    homepage = "http://uscilab.github.io/cereal";
    description = "A C++11 library for serialization";
    license = lib.licenses.bsd3;
  };
}
