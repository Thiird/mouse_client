# mouse_client

Desktop app to interact with my custom mouse

# Compile

For Windows:

```bash
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/mingw_64"
cmake --build .
```

For Linux:

```bash
cd build
cmake ..
make
```
