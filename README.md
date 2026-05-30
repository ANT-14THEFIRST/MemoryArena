NAS RANO


cat > BENCHMARK.md << 'EOF'
# Memory Arena Performance

| Allocator | 16B | 1KB | Batch 1000 |
|-----------|-----|-----|------------|
| LinearArena | 0.96 ns | 0.95 ns | 7.07 µs |
| PoolAllocator (64B) | — | — | 2.15 ns |
| FreeListArena | 6.22 ns | 6.76 ns | 1.95 µs |
| malloc/free | 9.38 ns | 11.24 ns | — |
| new/delete | 10.08 ns | 13.33 ns | 12.98 µs |

## How to run

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make arena_benchmark
./arena_benchmark --benchmark_out=results.json --benchmark_out_format=json
