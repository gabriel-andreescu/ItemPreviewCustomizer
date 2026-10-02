# ItemPreviewCustomizer

Customizes item preview zoom and rotation.

This project uses
[BethesdaModKit (BMK)](https://github.com/gabriel-andreescu/BethesdaModKit) for
project generation and development tooling.

## Development

```powershell
xmake
xmake package
```

- [Build instructions](https://github.com/gabriel-andreescu/BethesdaModKit/blob/main/docs/mod-authors/template/projects.md#build-a-generated-project)
- [Deployment and packaging](https://github.com/gabriel-andreescu/BethesdaModKit/blob/main/docs/mod-authors/tooling/packaging.md)
- [Formatting setup](https://github.com/gabriel-andreescu/BethesdaModKit/blob/main/docs/mod-authors/template/defaults.md#formatting)
- [Clang tooling](https://github.com/gabriel-andreescu/BethesdaModKit/blob/main/docs/mod-authors/tooling/clang.md)
- [Settings and MCM](https://github.com/gabriel-andreescu/BethesdaModKit/blob/main/docs/mod-authors/template/settings.md)

Run the configuration, model matching, and preview marker math tests with
`xmake test NativeTests/native`.

## CI

See
[workflow setup](https://github.com/gabriel-andreescu/BethesdaModKit/blob/main/docs/mod-authors/tooling/github-actions.md)
for build inputs and releases.
