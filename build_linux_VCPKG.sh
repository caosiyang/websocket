mkdir build_x64_linux_release_VCPKG
cd build_x64_linux_release_VCPKG
export CMAKE_TC=~/Projects/vcpkg/scripts/buildsystems/vcpkg.cmake
export VCPKGI=~/Projects/vcpkg/vcpkg

export VCPKG_FORCE_SYSTEM_BINARIES=1
${VCPKGI} install libevent[core,openssl,thread]  --triplet x64-linux --recurse

cmake  --triplet x64-linux DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TC} ..
cmake --build . --config Release --target lews
