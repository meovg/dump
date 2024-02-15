- Compile

```shell
g++ main.cpp --std=c++17 -I /usr/include/eigen3 -I ../include -O2 -o lenet5
```

- Run
```shell
./lenet5
```

- Format code (``clang-format`` required)
```shell
find ./ -iname *.h -o -iname *.cpp | xargs clang-format -i
```