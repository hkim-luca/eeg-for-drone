# CLAUDE.md — eeg-for-drone

## 소스 편집 완료 후 점검 (clang-tidy + microsoft style)

편집된 cpp/hpp에 clang-tidy 자동 수정과 microsoft 스타일 포맷을 적용한다.

```powershell
$files = git diff --name-only HEAD -- '*.cpp' '*.hpp'
& "C:\Program Files\LLVM\bin\clang-tidy.exe" --fix --fix-errors `
  --checks='readability-braces-around-statements,modernize-use-trailing-return-type,modernize-use-nullptr,modernize-use-auto,modernize-use-override,misc-unused-parameters,bugprone-*,performance-*' `
  $files -- -std=c++23 `
& "C:\Program Files\LLVM\bin\clang-format.exe" -i -style=microsoft $files
```
