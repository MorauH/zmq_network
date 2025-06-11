{
  description = "ZMQ networking library for Python and C++";
  
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        version = "0.1.0";
        pkgs = nixpkgs.legacyPackages.${system};
        pythonPkgs = pkgs.python312;
        in
      {
        packages = {
            # Python package
            python = pythonPkgs.pkgs.buildPythonPackage {
                pname = "zmq_network";
                version = version;
                src = ./python;
                
                format = "pyproject";

                nativeBuildInputs = [
                    pythonPkgs.pkgs.setuptools
                    pythonPkgs.pkgs.wheel
                ];
                
                propagatedBuildInputs = [
                    pythonPkgs.pkgs.pyzmq
                ];
                
                doCheck = false;  # disable tests
                
                meta = with pkgs.lib; {
                    description = "ZMQ networking library";
                    license = licenses.mit; 
                };
            };

            cpp = pkgs.stdenv.mkDerivation {
                name = "zmq-network";
                version = version;
                src = ./cpp;

                nativeBuildInputs = with pkgs; [
                    cmake
                    pkg-config
                ];

                buildInputs = with pkgs; [
                    zeromq
                    cppzmq
                    nlohmann_json
                ];

                cmakeFlags = [
                    "-DCMAKE_BUILD_TYPE=Release"
                    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
                ];

                installPhase = ''
                  make install
                '';
                
                doCheck = false;  # disable tests

                meta = with pkgs.lib; {
                    description = "ZMQ networking library";
                    license = licenses.mit; 
                    platforms = platforms.unix;
                };
            };

            # default to python
            default = self.packages.${system}.python;
        };
        
        devShells.default = pkgs.mkShell {

          buildInputs = with pkgs; [
            # Python
            (pythonPkgs.withPackages (ps: with ps; [
              pyzmq
            ]))

            # C++ development
            cmake
            pkg-config
            cppzmq
            nlohmann_json

            # General
            git
          ];

          PACKAGE_VERSION = version;

          #postVenvCreation = ''
          #  cd python
          #  uv sync
          #'';

          shellHook = ''
            Flake loaded.
            echo "PYTHONPATH=$PYTHONPATH:$(pwd)/python"
          '';
        };
      });
}