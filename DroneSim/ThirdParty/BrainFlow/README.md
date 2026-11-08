# ThirdParty/BrainFlow

BrainFlow C++ SDK를 빌드해서 이 폴더(`DroneSim/ThirdParty/BrainFlow/`)에서 아래 명령을 실행한다.

```
git clone https://github.com/brainflow-dev/brainflow
pushd brainflow
git apply ..\dspfilters-tr1.patch
cd tools
python -m pip install cmake
python build.py --generator "Visual Studio 18 2026" --arch x64 --msvc-runtime dynamic
popd
xcopy /E /I /Y brainflow\installed\inc inc
xcopy /I /Y brainflow\installed\lib\*.lib Win64
xcopy /I /Y brainflow\installed\lib\*.dll Win64
```

Linux (Ubuntu 24.04): 위처럼 clone 후

```
sudo apt install g++ make cmake
cd brainflow
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DBUILD_SYNCHRONI_SDK=OFF -DBRAINFLOW_COPY_TO_PACKAGE_DIRS=OFF -DCMAKE_SHARED_LINKER_FLAGS="-static-libstdc++ -static-libgcc"
cmake --build build-linux --target BoardController -j
cd ..
mkdir -p Linux && cp brainflow/compiled/libBoardController.so Linux/
```
