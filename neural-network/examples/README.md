- Compile

```shell
g++ [example filename] --std=c++17 -I [eigen3 include directory] -I ../headers -O2 -o [execution file]
```

- Format code (``clang-format`` required)
```shell
find ./ -iname *.h -o -iname *.cc | xargs clang-format -i
```