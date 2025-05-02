{
  description = "cpp extended meta state machine";
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = params@{ self, nixpkgs, ... }:
    params.flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfreePredicate = pkg: builtins.elem (nixpkgs.lib.getName pkg) [ "clion" ];
      };

      gcc_with_debinfo = pkgs.gcc14.cc.overrideAttrs(oldAttrs: { dontStrip=true; });
      stdenv_with_debinfo = pkgs.overrideCC pkgs.gcc14Stdenv gcc_with_debinfo;
      der = pkgs.gcc14Stdenv.mkDerivation {
        name = "xmsm";
        NIX_ENFORCE_NO_NATIVE = false;
        buildInputs = with pkgs;[ ];
        nativeBuildInputs = with pkgs;[
          gnumake gdb clang_20
          boost186
          llvmPackages_20.lld nodejs_22 nodePackages.http-server
        ];
        installPhase = "mkdir -p \"$out/include\" ";
        buildPhase = "make";
        meta.description = "extended sate machied. it suppurs linking beween few machines.";
        src = ./.;
      };
    in rec {
      #TODO: unset SOURCE_DATE_EPOCH for devShell (for provide random value in compile time)
      devShell = der.overrideAttrs(finalAttrs: previousAttrs: {
          nativeBuildInputs =  previousAttrs.nativeBuildInputs ++ [ pkgs.jetbrains.clion ];
        });
      packages.default = der;
      packages.xmsm = der;
      defaultPackage = der;
    });
}
