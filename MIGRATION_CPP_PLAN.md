# EvelentWalker: план миграции C# → C++

Документ описывает перенос проекта EvelentWalker (форк CodeWalker) с C#/.NET на C++.
Здесь же ведётся трекинг статуса: что сделано, что в работе, что не начато.

## Масштаб

- ~300 000 строк C# в 393 файлах.
- Решение состоит из проектов:
  - `EvelentWalker.Core` — ядро: парсинг форматов RAGE (RPF, ресурсы, мета-типы), шифрование, утилиты.
  - `EvelentWalker` — главное WinForms/WPF приложение.
  - `EvelentWalker.WinForms`, `EvelentWalker.RPFExplorer`, `EvelentWalker.Peds`, `EvelentWalker.Vehicles`, `EvelentWalker.ModManager`, `EvelentWalker.Gen9Converter`, `EvelentWalker.ErrorReport`, `EvelentWalker.Shaders` — UI/вспомогательные.
- Внешние зависимости C#: SharpDX (Direct3D11, Direct2D1, Mathematics, XAudio2, XInput, D3DCompiler), WinForms, WPF, DockPanelSuite, FCTB, Costura.Fody.

## Принципы переноса

1. **Снизу вверх.** Сначала переносим слои без зависимостей (математика, бинарный I/O, хеши), затем форматы, затем рендер, в последнюю очередь — UI.
2. **Поэтапная проверка.** Каждый перенесённый модуль покрывается минимальным тестом и компилируется в общую статическую библиотеку `evw_core`.
3. **Замена зависимостей:**
   - `SharpDX.Mathematics` → собственный лёгкий заголовок `math/` (Vector2/3/4, Matrix, Quaternion) или GLM.
   - `SharpDX.Direct3D11` → нативный Direct3D 11 (d3d11.h) либо переносимо через abstraction layer.
   - `System.IO.Stream` → `std::istream`/`std::ostream` + обёртки над буфером в памяти.
   - WinForms/WPF → Qt или Dear ImGui (решение на этапе UI; ядро от UI не зависит).
4. **Целевая сборка:** C++20, CMake, MSVC (Windows x64).

## Этапы и статус

Легенда: [x] готово · [~] в работе · [ ] не начато

### Этап 0 — Инфраструктура
- [x] Каркас CMake-проекта (`cpp/`)
- [x] Структура каталогов под зеркало `Core`
- [ ] Подключение тестового фреймворка (минимальный self-test раннер)
- [ ] CI/скрипт сборки

### Этап 1 — Математика (замена SharpDX.Mathematics)
- [x] Vector2 / Vector3 / Vector4
- [x] Matrix (4x4)
- [~] Quaternion
- [ ] BoundingBox / BoundingSphere / Ray / Plane
- [ ] Полный набор операций (transform, intersect и т.д.)

### Этап 2 — Фундамент Core/Utils
- [x] Jenkins hash (`Jenk.cs` → `jenk.h/.cpp`)
- [x] JenkIndex (потокобезопасный словарь хешей)
- [x] DataReader / DataWriter (`Data.cs` → `data.h/.cpp`)
- [x] AES-256 ECB с нуля (`aes.h/.cpp`, проверено по вектору FIPS-197 C.3)
- [x] GTACrypto: AES-обёртки + NG-расшифровка (`gtacrypto.h/.cpp`)
- [x] GTA5Hash.CalculateHash + выбор NG-ключа (getNGKey)
- [ ] GTACrypto: NG-**шифрование** (нужны RandomGauss-solver + генерация LUT)
- [x] GTA5Keys: распаковка таблиц ключей (CryptoIO ReadNgKeys/ReadNgTables) + magic-блоб
- [x] GTA5Keys: полный пайплайн `magic.dat` (de-scramble + AES + inflate + unpack)
- [x] .NET-совместимый Random (нужен для magic.dat) — проверен по эталону System.Random
- [x] GTA5Keys: извлечение AES-ключа из exe — не сделано (ключ подаётся снаружи)
- [x] DDSIO: генерация `.dds` (заголовок + DX10-ext для BC7) из текстуры — проверено
- [x] DXT-декодирование DXT1/DXT3/DXT5 → RGBA8 (`dxt_decode`) — проверено на блоках
- [ ] Декодирование ATI1/ATI2/BC7 → RGBA

### Этап 3 — Контейнеры ресурсов / сжатие
- [x] DEFLATE-распаковка с нуля (`inflate.h/.cpp`, RFC 1951; проверено вектором из .NET)
- [ ] DEFLATE-сжатие (для записи ресурсов)
- [x] ResourceDataReader: виртуальная адресация system/graphics (0x50000000/0x60000000)
- [x] ResourceFileBase / ResourcePagesInfo (заголовок ресурса) + ReadBlockAt/ReadStringAt
- [x] Размеры сегментов и версия ресурса у записи (systemSize/graphicsSize/resourceVersion)
- [x] Base-types массивов: SimpleArray, SimpleList64, PointerArray64, PointerList64
- [ ] ResourceBuilder (запись/сборка ресурсов) / остальные base-types

### Этап 4 — RPF-архивы
- [x] Структуры записей (Directory/Binary/Resource) + парсинг TOC (`rpf.h/.cpp`)
- [x] Расшифровка TOC и записей (NONE/OPEN/AES/NG) + распаковка (raw DEFLATE)
- [x] Извлечение файлов (`extractFile`), построение дерева путей, page-flags размеры
- [x] Источники данных: файловый и in-memory (`FileDataSource`/`MemoryDataSource`)
- [x] RpfManager (рекурсивный обход вложенных RPF, индексация путей, поиск, extract по пути)
- [ ] GameFile / GameFileCache

### Этап 5 — Мета-типы
- [ ] Meta / MetaTypes / MetaNames / MetaXml / MetaBuilder
- [ ] Pso / PsoTypes / PsoBuilder
- [ ] Rbf / XmlMeta / XmlPso / XmlRbf

### Этап 6 — Форматы файлов (FileTypes/*)
- [x] Ytd (текстуры): TextureDictionary / Texture / TextureData (legacy/non-gen9) — проверено
- [ ] gen9-вариант текстур (Enhanced)
- [~] Модели: Ydr/Ydd/Yft (Drawable/Fragment)
  - [x] Геометрия: VertexDeclaration / VertexBuffer / IndexBuffer (legacy) — проверено
  - [x] DrawableGeometry / DrawableModel (сборка сеток: VB+IB+shaderId+AABB) — проверено
  - [ ] ShaderGroup / Skeleton / Drawable / Fragment (Ydr/Yft верхний уровень)
- [ ] Карты: Ymap/Ytyp
- [ ] Остальные ~35 форматов

### Этап 6.5 — Приложение
- [x] CLI `evw_cli` (scan / extract / ytd) поверх ядра — связывает все слои, собирается

### Этап 7 — Рендеринг
- [ ] Абстракция устройства Direct3D 11
- [ ] Шейдеры, Renderable, Renderer

### Этап 8 — UI
- [ ] Выбор фреймворка (Qt / ImGui)
- [ ] Перенос форм поверх готового ядра

## Текущий прогресс (обновляется по ходу)

- Заложен C++ проект `cpp/` (CMake, библиотека `evw_core`, исполняемый self-test `evw_tests`).
- Перенесены и проверены:
  - математические типы (Vector2/3/4, Matrix, Quaternion);
  - Jenkins hash + JenkIndex;
  - DataReader / DataWriter;
  - AES-256 ECB с нуля (KAT по FIPS-197);
  - GTACrypto: AES-обёртки и NG-расшифровка + выбор NG-ключа;
  - DEFLATE-распаковка (RFC 1951, проверена вектором из .NET DeflateStream);
  - .NET-совместимый System.Random (проверен по эталону);
  - распаковка таблиц ключей и полный пайплайн `magic.dat` (de-scramble + AES + inflate + unpack);
  - **чтение RPF-архивов**: парсинг TOC, расшифровка (AES/NG), распаковка (DEFLATE),
    извлечение файлов, построение дерева путей (проверено на синтетическом RPF7);
  - **RpfFile.scanStructure**: рекурсивный обход вложенных `.rpf` (общий источник данных);
  - **RpfManager**: сканирование папки игры, индекс путей, поиск и извлечение файла по пути
    (проверено на временном `.rpf` на диске через `FileDataSource` + `std::filesystem`);
  - **система ресурсов**: `ResourceDataReader` с виртуальной адресацией system/graphics,
    чтение заголовка ресурса (`ResourceFileBase`/`ResourcePagesInfo`) по указателям,
    `ReadBlockAt`/`ReadStringAt`/массивы (проверено на синтетическом ресурсе);
  - **base-types массивов**: SimpleArray / SimpleList64 / PointerArray64 / PointerList64
    (шаблоны, проверены чтением вложенных списков и разыменованием указателей);
  - **формат текстур (YTD)**: TextureDictionary / Texture / TextureData (legacy) —
    первый реальный `Y*`-парсер, проверен на синтетическом ресурсе (имя, размеры,
    формат, мип-данные из graphics-сегмента, lookup по хешу);
  - **CLI `evw_cli`** (scan / extract / ytd + экспорт DDS) — связывает все слои в рабочий инструмент;
  - **экспорт DDS** (`ddsio`): валидный `.dds` (заголовок 124B + DX10-ext для BC7) из текстуры,
    проверено по байтам заголовка (magic, размеры, FourCC, данные);
  - **декодирование DXT** (`dxt_decode`): DXT1/DXT3/DXT5 → RGBA8 (проверено на блоках
    с известными цветами/альфой), плюс 32-битные форматы;
  - **геометрия моделей** (`geometry`): VertexDeclaration / VertexBuffer / IndexBuffer (legacy) —
    проверено на синтетическом ресурсе (stride, count, declaration, вершинные байты, индексы);
  - **сетки моделей** (`model`): DrawableGeometry / DrawableModel — сборка геометрий
    (VB+IB+shaderId+AABB), проверено на синтетической модели с одной геометрией.

### Сборка приложения

`build.cmd` собирает три цели: `evw_core` (библиотека), `evw_tests` (тесты),
`evw_cli` (CLI). Бинарь: `cpp/build/Debug/evw_cli.exe`.
- **Проверено:** проект собирается под MSVC (C++20); `build.cmd`/`build.ps1` проходят
  конфигурацию → сборку → тесты; self-test зелёный (включая AES known-answer test).
- **Скрипты сборки:** `cpp/build.cmd` и `cpp/build.ps1` (configure + build + ctest).
- Следующий шаг: загрузка ключей (`magic.dat`), затем контейнеры ресурсов и чтение RPF.

### Сборка и запуск тестов

```cmd
cd cpp
build.cmd            REM Debug по умолчанию: конфигурация + сборка + тесты
build.cmd Release
```

или вручную:

```cmd
cd cpp
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Заметки по различиям C# → C++

- `uint`/`int` C# → `uint32_t`/`int32_t`. Важно сохранять беззнаковую семантику сдвигов в хешах.
- `byte[]` → `std::vector<uint8_t>` или `std::span<const uint8_t>`.
- `Stream` чтение little/big-endian → ручная конвертация байт; на x86/x64 хост — little-endian.
- `string` (UTF-16 .NET) → `std::string` (UTF-8) для хешей и имён.
- `Dictionary` → `std::unordered_map`; `lock` → `std::mutex`.
