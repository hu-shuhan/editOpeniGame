import os
import sys
import subprocess
import shutil
import argparse
import tarfile


def run_command(command, shell=False):
    """Executes a command and exits if it fails."""
    print(f"--- Running command: {' '.join(command)} ---")
    try:
        subprocess.run(command, check=True, shell=shell)
        print("--- Command successful ---\n")
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"--- Command failed: {e} ---")
        sys.exit(1)


def is_multi_config_generator(gen_name: str) -> bool:
    gen_name = (gen_name or "").lower()
    # Visual Studio and Xcode are multi-config; Ninja/Makefiles are single
    return "visual studio" in gen_name or "xcode" in gen_name or "multi-config" in gen_name


def build_and_package(build_config, args):
    """Runs the full CMake build, install, and packaging process for a given configuration."""
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    build_dir_name = build_config["name"]
    build_dir = os.path.join(project_root, build_dir_name)
    install_dir = os.path.join(build_dir, "install")
    package_source_dir = os.path.join(build_dir, "iGameCore")
    archive_name = os.path.join(project_root, f"{build_dir_name}.tar.gz")

    print(f"--- Starting build for configuration: {build_dir_name} ---")
    print(f"Project root: {project_root}")
    print(f"Build directory: {build_dir}")

    # Clean build directory only if not reusing
    if os.path.exists(build_dir) and not args.reuse_build:
        print(f"Removing existing build directory: {build_dir}")
        shutil.rmtree(build_dir)

    generator = args.generator
    multi_config = is_multi_config_generator(generator)
    build_type = args.build_type

    cmake_command = ["cmake"]
    if generator:
        cmake_command += ["-G", generator]

    # Add user cmake options from build_config
    cmake_command += build_config.get("cmake_opts", [])

    # Single-config generators need CMAKE_BUILD_TYPE
    if build_type and not multi_config:
        cmake_command += [f"-DCMAKE_BUILD_TYPE={build_type}"]

    cmake_command += [
        "-S", project_root,
        "-B", build_dir,
        f"-DENABLE_CGNS_MODULE={'ON' if args.enable_cgns else 'OFF'}",
        f"-DENABLE_NASTRAN_MODULE={'ON' if args.enable_nastran else 'OFF'}",
        f"-DENABLE_LIBTORCH_MODULE={'ON' if args.enable_libtorch else 'OFF'}",
        f"-DENABLE_GPSCUDA_MODULE={'ON' if args.enable_gpscuda else 'OFF'}",
        f"-DENABLE_QT_MODULE={'ON' if args.enable_qt else 'OFF'}",
    ]

    run_command(cmake_command)

    # --- Build ---
    cpu_cores = os.cpu_count() or 1
    build_cmd = ["cmake", "--build", build_dir, "--parallel", str(cpu_cores)]
    if multi_config and build_type:
        build_cmd += ["--config", build_type]
    print(
        f"--- Starting build ({'multi-config' if multi_config else 'single-config'}) with {cpu_cores} parallel jobs ---")
    run_command(build_cmd)

    # --- Install ---
    install_cmd = ["cmake", "--install", build_dir]
    if multi_config and build_type:
        install_cmd += ["--config", build_type]
    print("--- Starting install ---")
    run_command(install_cmd)

    if args.skip_package:
        print("--- Skipping packaging as requested ---")
        return

    # --- Package ---
    if not os.path.exists(install_dir):
        print(f"Error: Install directory '{install_dir}' not found after build. Aborting packaging.")
        return

    print(f"Moving '{install_dir}' to '{package_source_dir}' for packaging.")
    if os.path.exists(package_source_dir):
        shutil.rmtree(package_source_dir)
    shutil.move(install_dir, package_source_dir)

    print(f"Creating archive: {archive_name}")
    with tarfile.open(archive_name, "w:gz") as tar:
        tar.add(package_source_dir, arcname=os.path.basename(package_source_dir))

    print(f"\n--- Packaging complete for {build_dir_name}! ---")
    print(f"Archive created at: {archive_name}")


def main():
    parser = argparse.ArgumentParser(description="Automated build and packaging script for iGameCore.")
    parser.add_argument('--enable-cgns', action='store_true', help="Enable CGNS ThirdParty Module.")
    parser.add_argument('--enable-nastran', action='store_true', help="Enable Nastran Lib ThirdParty Module.")
    parser.add_argument('--enable-libtorch', action='store_true', help="Enable Libtorch ThirdParty Module.")
    parser.add_argument('--enable-gpscuda', action='store_true', help="Enable GPS CUDA Module.")
    parser.add_argument('--enable-qt', action='store_true', help="Build Qt/GUI Module.")
    parser.add_argument('--build-type', default='Debug',
                        help="Build type (Debug/Release/RelWithDebInfo/MinSizeRel). For single-config generators.")
    parser.add_argument('--generator', default='', help="CMake generator to use (e.g. Ninja, Visual Studio 17 2022).")
    parser.add_argument('--reuse-build', action='store_true', help="Reuse existing build directory (no clean).")
    parser.add_argument('--skip-package', action='store_true', help="Skip packaging stage.")
    args = parser.parse_args()

    build_configs = []
    if sys.platform.startswith("linux"):
        print("--- Detected Linux platform. Configuring GCC builds. ---")
        build_configs.append({
            "name": "cmake-autobuild-gcc11",
            "cmake_opts": [
                "-DCMAKE_C_COMPILER=/usr/bin/gcc-11",
                "-DCMAKE_CXX_COMPILER=/usr/bin/g++-11"
            ]
        })
        build_configs.append({
            "name": "cmake-autobuild-gcc13",
            "cmake_opts": [
                "-DCMAKE_C_COMPILER=/usr/bin/gcc-13",
                "-DCMAKE_CXX_COMPILER=/usr/bin/g++-13"
            ]
        })
    elif sys.platform == "win32":
        print("--- Detected Windows platform. Configuring MSVC build. ---")
        build_configs.append({"name": "cmake-autobuild-msvc", "cmake_opts": []})
    else:
        print(f"--- Unsupported platform: {sys.platform}. Exiting. ---")
        sys.exit(1)

    for config in build_configs:
        build_and_package(config, args)


if __name__ == "__main__":
    main()
