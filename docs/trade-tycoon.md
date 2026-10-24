# Trade Tycoon (личный форк OpenTTD)

Ветка `trade-tycoon`: OpenTTD + экономические механики Capitalism Lab. Upstream: `upstream` = OpenTTD/OpenTTD.

## Сборка

```sh
python3 bundled/fetch_content.py      # OpenGFX + NewGRF из BaNaNaS -> bundled/content/ (не в git)
python3 ../tools/products_grf/build.py   # товары Capitalism Lab -> bundled/content/newgrf/tradetycoon_products.grf (из вашей копии игры, не в git)
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
| До 240 типов груза (было 64): каждый товар Capitalism — отдельный груз | `src/core/wide_bitset_type.hpp`, `src/cargo_type.h`, сейвы (`WideCargoTypes`) |
| Месторождения компаний: запас, качество, суточная добыча, исчерпание (механика 12). Параметры — `tradetycoon_economy.json` рядом с NewGRF товаров | `src/tradetycoon/`, `src/industry_cmd.cpp`, `src/industry_gui.cpp`, сейвы (чанк `TTDP`) |
| Фабрики компаний с рецептами Capitalism, качество в партиях груза (механики 2, 3) | `src/tradetycoon/factory.cpp`, `src/cargopacket.*`, `src/economy.cpp`, `src/station_cmd.cpp`, команда `SetFactoryRecipe`, `AIIndustry.SetFactoryProduct`, сейвы (`CargoQuality`, чанк `TTFC`) |

## Правила

- Данные Capitalism (таблицы, тексты, графика) в этот репозиторий не коммитятся. Они лежат в рабочей папке проекта (`catalog/`, `research/`).
- Изменения в файлах upstream делать минимальными, а новый код выносить в отдельные файлы, чтобы merge с upstream оставался простым.
- Новые строки добавлять в `english.txt` и `russian.txt`.
