# 1BCR - C

I will iterate on the [1M file](./measurements.txt)
Each iteration will be tag. Each iteration result (output & elapsed time) can be found [here](./result.txt).

## Optimization

- [x] optimized size buffer file read
- [x] convert float to int
- [x] mmap entire file
- [x] use hashmap
- [ ] unrolled parsing & hash gen
- [ ] reduce CPU branching
- [ ] eliminate use of key string dynamic allocation
- [ ] SIMD
- [ ] multithreading

## Logs

- naive implementation: [@058a903](https://github.com/melvi-l/1bcr-c/blob/naive/result.txt#L421)
- manual file read: [@c6933ed](https://github.com/melvi-l/1bcr-c/blob/manual_read/result.txt#L421)
- convert float to int: [5d87301](https://github.com/melvi-l/1bcr-c/blob/float_to_int/result.txt#L421)

## TODO

- measure computer throughput
