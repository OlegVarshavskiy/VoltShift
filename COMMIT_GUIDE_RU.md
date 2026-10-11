# Как перенести и закоммитить VoltShift v0.3

Изменения подготовлены в отдельной копии проекта. Исходная папка на диске D: не редактировалась. Коммит и отправка на GitHub не выполнялись.

## 1. Перенос в актуальный репозиторий

Откройте PowerShell в корне вашего актуального репозитория EnergySaver/VoltShift и выполните:

```powershell
git status --short
git log -1 --oneline
```

Базовый коммит предоставленной версии: `0ba52a1`. Если у вас появились новые изменения, сначала сравните их с подготовленными файлами и объедините вручную. Сохраните незакоммиченные изменения до копирования.

Из подготовленной папки EnergySaver перенесите в актуальный репозиторий:

- `src/main.c` — обновлённый файл;
- `src/gpu.c`, `src/battery.c`, `src/power_plan.c`, `src/capabilities.c` — новые модули;
- `include/gpu.h`, `include/battery.h`, `include/power_plan.h`, `include/capabilities.h` — новые заголовки;
- `tests/test_diagnostics.c`, `build.ps1`, `README.md`, `VALIDATION.md`, `COMMIT_GUIDE_RU.md`.

Существующие `src/hardware.c`, `include/hardware.h` и `.gitignore` не изменены. Не копируйте `.git` и старый корневой `voltshift.exe` из исходной версии. Новая программа собирается в `build/voltshift.exe`.

## 2. Сборка и проверка

```powershell
.\build.ps1 -Test
.\build\voltshift.exe
git diff --check
git diff
```

Ожидается сообщение PASS, заголовок v0.3 и реальные данные вашей системы. Неизвестные значения или отсутствие батареи допустимы. При ошибках чтения основной диагностики приложение выводит код Windows и возвращает 1.

## 3. Коммит

```powershell
git switch -c feature/voltshift-v0.3
git add src/main.c src/gpu.c src/battery.c src/power_plan.c src/capabilities.c
git add include/gpu.h include/battery.h include/power_plan.h include/capabilities.h
git add tests/test_diagnostics.c build.ps1 README.md VALIDATION.md COMMIT_GUIDE_RU.md
git diff --cached --check
git diff --cached --stat
git diff --cached
git commit -m "feat: add read-only GPU, battery and power diagnostics for v0.3"
```

Проверьте, что в staged diff нет чужих изменений, исполняемых файлов и каталога build. Если ветка уже существует, выберите другое имя или перейдите в неё.

## 4. Отправка, когда будете готовы

```powershell
git remote -v
git push -u origin feature/voltshift-v0.3
```

Эта команда предполагает, что `origin` указывает на ваш актуальный GitHub-репозиторий. При другом имени remote или ветки замените соответствующее значение. Архив результата содержит исходники без Git-истории и без готовых исполняемых файлов.
