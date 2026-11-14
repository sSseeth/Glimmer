# Development guide

## Presets

| Preset | Use it for |
|---|---|
| `core` | Library and tests only. No Qt needed; the fastest loop. |
| `dev` | The full app, Debug, AddressSanitizer + UBSan. Your default. |
| `dev-whisper` | `dev` plus whisper.cpp transcription (slower first build). |
| `release` | Optimised build with transcription, no tests. |

```sh
cmake --preset core && cmake --build --preset core && ctest --preset core
```

Run a single test while iterating:

```sh
./build/core/tests/glimmer_tests --gtest_filter='EntryRepositoryTest.*'
```

## Neovim

Everything goes through **clangd**, which reads `compile_commands.json`.

1. Install `clangd`, `clang-format` and `codelldb` (or use `gdb`), e.g. with
   `:MasonInstall clangd clang-format codelldb`.
2. Symlink the compile database into the repo root (it's gitignored):
   ```sh
   ln -sf build/dev/compile_commands.json .
   ```
3. A minimal `nvim-lspconfig` setup:
   ```lua
   require("lspconfig").clangd.setup({
     cmd = { "clangd", "--background-index", "--clang-tidy",
             "--header-insertion=never", "--completion-style=detailed" },
   })
   ```
   `--clang-tidy` shows the repo's `.clang-tidy` warnings inline.
4. Format on save with conform.nvim (it picks up `.clang-format`):
   ```lua
   require("conform").setup({
     formatters_by_ft = { cpp = { "clang_format" } },
     format_on_save = { timeout_ms = 1000 },
   })
   ```
5. Debugging with nvim-dap and codelldb:
   ```lua
   local dap = require("dap")
   dap.configurations.cpp = {
     {
       name = "glimmer (throwaway diary)",
       type = "codelldb",
       request = "launch",
       program = "${workspaceFolder}/build/dev/app/glimmer",
       args = { "--data-dir", "/tmp/glimmer-dev" },
       cwd = "${workspaceFolder}",
     },
     {
       name = "unit tests",
       type = "codelldb",
       request = "launch",
       program = "${workspaceFolder}/build/dev/tests/glimmer_tests",
       cwd = "${workspaceFolder}",
     },
   }
   ```

Useful clangd commands: `:ClangdSwitchSourceHeader` (jump between `.hpp` and
`.cpp`), `gd` (go to definition), `K` (hover docs; the `///` comments in the
headers show up here).

## Sanitizers

The `dev` and `core` presets build with `-fsanitize=address,undefined`. A
memory bug aborts with a stack trace pointing at the line. Don't turn the
sanitizers off to make a crash go away; the crash is the bug.

Qt and system libraries (fontconfig, audio backends) keep some allocations
alive until exit, which LeakSanitizer reports when you close the app. Leak
reports for `core/` tests are real; for the GUI, run with
`ASAN_OPTIONS=detect_leaks=0 ./build/dev/app/glimmer` and rely on the core
tests for leak checking.

## Never test against your real diary

Use `--data-dir /tmp/glimmer-dev` (the nvim-dap config above already does).
Delete that folder to start fresh.

## Adding a database migration

1. Append a new SQL string to `kMigrations` in `core/src/migrations.cpp`.
   **Never edit an existing one**: users' databases have already run it.
2. Add a test in `tests/migrations_test.cpp` or the relevant repository test.
3. Mention it under "Changed" in `CHANGELOG.md`.

## Formatting and static analysis

```sh
scripts/format.sh                           # clang-format in place
run-clang-tidy -p build/core 'core/src/.*'  # same check CI runs
```
