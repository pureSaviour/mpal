# MPAL benchmarks

The benchmark target is optional so normal library consumers do not download
Google Benchmark. The repository preset configures Release mode, disables tests
and the example executable, and builds only the benchmark target:

```powershell
cmake --workflow --preset benchmark
.\out\build\benchmark\benchmarks\mpal_benchmarks.exe
```

The test-only Debug workflow is:

```powershell
cmake --workflow --preset test
```

For a single-config generator such as Ninja or MinGW Makefiles:

```powershell
cmake -S . -B build-release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_TESTING=OFF `
  -DMPAL_BUILD_BENCHMARKS=ON
cmake --build build-release --target mpal_benchmarks
.\build-release\benchmarks\mpal_benchmarks.exe
```

For a multi-config generator such as Visual Studio:

```powershell
cmake -S . -B build-vs -DBUILD_TESTING=OFF -DMPAL_BUILD_BENCHMARKS=ON
cmake --build build-vs --config Release --target mpal_benchmarks
.\build-vs\benchmarks\Release\mpal_benchmarks.exe
```

Use Release builds for comparisons. Debug timings are useful only for checking
that benchmarks execute; they do not represent the library's optimized speed.
