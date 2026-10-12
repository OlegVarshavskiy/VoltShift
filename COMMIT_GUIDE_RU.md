# Коммит VoltShift v0.5

Сначала объедините файлы пакета с существующим проектом, сохранив .git. Все команды ниже выполняются в UCRT64 Bash из корня EnergySaver.

~~~bash
bash build.sh test
bash build.sh live-test
~~~

live-test временно меняет настройки и пытается восстановить их. Не называйте проверку успешной, если она завершилась ошибкой; сохраните её каталог восстановления при ошибке восстановления.

Посмотрите изменения и добавьте только исходники, тесты и документацию:

~~~bash
git status --short
git diff --check
git diff --stat
git add src include tests build.sh README.md QUICK_START.md VALIDATION.md COMMIT_GUIDE_RU.md
git diff --cached --stat
git commit -m "Add reversible battery-first save profile and display recovery"
~~~

Проверьте список перед коммитом. Не добавляйте build/, .exe и файлы recovery session. Если нужен push, используйте обычную команду для вашей уже настроенной ветки/remote. Пакет не изменяет Git-историю и сам не создаёт коммит.
