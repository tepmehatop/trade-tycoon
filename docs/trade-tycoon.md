# Trade Tycoon (личный форк OpenTTD)

Ветка `trade-tycoon`: OpenTTD + экономические механики Capitalism Lab. Upstream: `upstream` = OpenTTD/OpenTTD.

## Сборка

```sh
python3 bundled/fetch_content.py      # OpenGFX + NewGRF из BaNaNaS -> bundled/content/ (не в git)
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j 16
build/openttd
```

Скачанные версии и SHA-256 фиксируются в `bundled/content.lock.json`.

## Отличия от upstream

| Что | Где |
|---|---|
| Собираются только английский (мастер) и русский языки. Все переводы: `-DOPTION_ALL_LANGUAGES=ON` | `src/lang/CMakeLists.txt`, `cmake/Options.cmake` |
| Встроенный контент: OpenGFX, ITL Houses, FIRS 5, Iron Horse 4, Road Hog, CHIPS. Копируется в `build/{baseset,newgrf}` | `bundled/` |
| При первом запуске (в конфиге нет `[newgrf]`) встроенные NewGRF включаются для новых игр | `src/bundled_content.cpp`, `src/settings.cpp` |

## Правила

- Данные Capitalism (таблицы, тексты, графика) в этот репозиторий не коммитятся. Они лежат в рабочей папке проекта (`catalog/`, `research/`).
- Изменения в файлах upstream делать минимальными, а новый код выносить в отдельные файлы, чтобы merge с upstream оставался простым.
- Новые строки добавлять в `english.txt` и `russian.txt`.
