{
  description = "C++ with OpenCV environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.05";
  };

  outputs = { self , nixpkgs , ... }: let
    system = "x86_64-linux";
  in {
    devShells."${system}".default = let
      pkgs = import nixpkgs {
        inherit system;
      };
    in pkgs.mkShell {
      packages = with pkgs; [
        cmake
        gdb
        clang-tools
        (opencv.override { enableGtk3 = true; })
        # mosquitto
        # paho-mqtt-c
        # paho-mqtt-cpp
        # openssl
      ];
    };
  };
}
