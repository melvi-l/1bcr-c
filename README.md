# 1BCR - C

I will iterate on the [1M file](./measurements.txt)
Each iteration will be tag. Each iteration result (output & elapsed time) can be found [here](./result.txt).

## Optimization

- [x] optimized size buffer file read
- [x] convert float to int
- [x] mmap entire file
- [x] use hashmap
- [x] unrolled parsing & hash gen
- [x] eliminate use of key string dynamic allocation
- [ ] reduce CPU branching
- [ ] multithreading
- [ ] 3 cursor ILP (nl scanner, station parser, temperature parser + insert)

- [ ] SIMD

## Logs

- naive implementation: [@058a903](https://github.com/melvi-l/1bcr-c/blob/naive/result.txt#L421)
- manual file read: [@c6933ed](https://github.com/melvi-l/1bcr-c/blob/manual_read/result.txt#L421)
- convert float to int: [@5d87301](https://github.com/melvi-l/1bcr-c/blob/float_to_int/result.txt#L421)
- hashmap & mmap: [@c50a4e9](https://github.com/melvi-l/1bcr-c/blob/hashmap_mmap/result.txt#L421)

## Aggressive optimization compiler flag

Absolutely have no idea what this means.

```sh
gcc -O3 -march=native -flto -fomit-frame-pointer -funroll-loops -fstrict-aliasing \
    -fno-plt -fno-semantic-interposition -ffast-math -funsafe-math-optimizations  \
    -fno-math-errno -fno-asynchronous-unwind-tables -fno-unwind-tables            \
    -fno-stack-protector -fmerge-all-constants -ftracer -fipa-pta -fipa-cp-clone  \
    -falign-functions=32 -falign-loops=32 -o main main.c
```

None of those words appear in the Bible.

## TODO

- measure computer throughput
