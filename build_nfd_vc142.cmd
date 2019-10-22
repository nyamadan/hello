PUSHD nativefiledialog\build\vs2010
msbuild NativeFileDialog.sln -m -t:Build -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v142
POPD
COPY /Y nativefiledialog\build\lib\Release\x64\nfd.lib nfd.lib
