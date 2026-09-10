# Verification report

This report is generated from files and recorded command results. It does not infer that a check passed from the presence of source code.

## Recorded checks

| Check | Status | Exit code |
|---|---|---|
| fetch-json | failed | 6 |
| configure-domain | not run or no result recorded | None |
| test-domain | not run or no result recorded | None |
| configure-desktop | not run or no result recorded | None |
| capture-tanks | not run or no result recorded | None |
| capture-shop | not run or no result recorded | None |
| capture-collection | not run or no result recorded | None |
| capture-settings | not run or no result recorded | None |
| performance-40-fish | not run or no result recorded | None |
| configure-sanitize | not run or no result recorded | None |
| test-sanitize | not run or no result recorded | None |
| asset-integrity | not run or no result recorded | None |

## Scope and limitations

The full requested scope is not certified complete. The feature matrix identifies remaining systems and verification gaps. Reference fixtures use synthetic review state unless specifically measured from the supplied images. No comparison to an unavailable running original application is claimed.

The available build host is Linux-6.18.35-x86_64-with-glibc2.41. No Android or iOS device testing, signing, installation, native safe-area validation, or mobile frame-rate certification is asserted.

Any performance JSON describes only the named native-host scenario and its recorded measurement method. A software or dummy SDL driver is not a representative mobile GPU.

A capture file proves that a rendering command produced an image. It does not prove accurate visual composition, correct interaction, or a complete game.

Font binaries are excluded from the delivered archive. The setup script installs fonts on the developer's own machine.

Raw logs and JSON reports are retained under `evidence/`.
