mkdir build_x64_vs2019_release_VCPKG
cd build_x64_vs2019_release_VCPKG
IF EXIST C:/vcpkg/ (
set CMAKE_TC=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
set VCPKGI=C:/vcpkg/vcpkg.exe
) ELSE (
set CMAKE_TC=C:/VS_Projects/vcpkg/scripts/buildsystems/vcpkg.cmake
set VCPKGI=C:/VS_Projects/vcpkg/vcpkg.exe
)
echo %CMAKE_TC%

%VCPKGI% install libevent[core,openssl,thread]:x64-windows-static

cmake -G"Visual Studio 16 2019" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TC% ..
cmake --build . --config Release --target lews -- /m
