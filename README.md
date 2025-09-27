# Mouse client

Desktop app to interact with my custom mouse

# Compile

For Windows:

```bash
cd build

# Release
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/mingw_64"
# Debug
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/mingw_64" -DCMAKE_BUILD_TYPE=Debug

cmake --build .
cmake --build . --target clean

```

For Linux:

```bash

```

# Run

For Windows:

```bash
cd build
C:\Qt\<version>\mingw_64\bin\windeployqt.exe .\mouse_client.exe
.\mouse_client.exe
```

For Linux:

```bash

```
