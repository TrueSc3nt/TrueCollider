# Release Notes Template

Use this template when drafting a new GitHub release. The release workflow is triggered by pushing a tag `vX.Y.Z`.

```markdown
## TrueCollider vX.Y.Z

### Highlights
- One-line summary of the biggest change in this release.

### New Features
- Feature one
- Feature two

### Performance
- Speed improvement in mode X
- Lower memory usage for Y target set size

### Build & Portability
- Linux x64 build
- Windows x64 .exe cross-compiled with MinGW
- Linux aarch64 build (Termux compatible when built on device)
- CMake support for `CPU_GRP_SIZE` tuning

### Bug Fixes
- Fix one
- Fix two

### Assets
| Asset | Platform | Notes |
|-------|----------|-------|
| `keyhunt-linux-x64` | Linux x86_64 | Dynamic link, glibc |
| `keyhunt-windows-x64.exe` | Windows x64 | Static, MinGW-w64 |
| `keyhunt-linux-aarch64` | Linux ARM64 / Termux | Build from source for Termux; cross-compiled ELF for CI |

### SHA-256 Checksums
```
keyhunt-linux-x64:        SHA256...
keyhunt-windows-x64.exe:  SHA256...
keyhunt-linux-aarch64:    SHA256...
```

### Breaking Changes
- None (or list them)

### Known Issues
- GPU backend is experimental scaffolding only.
- On-device Termux builds require `pkg install cmake make clang`.

### Full Changelog
https://github.com/TrueSc3nt/TrueCollider/compare/vPREV...vX.Y.Z
```

## Creating a release

```bash
# 1. Update version strings if necessary
git add .
git commit -m "Release vX.Y.Z"

# 2. Tag and push
git tag vX.Y.Z
git push origin main --tags

# 3. GitHub Actions will build the three assets and create a draft release.
# 4. Edit the draft, paste the notes above, and publish.
```
