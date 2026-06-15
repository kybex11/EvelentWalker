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
- [x] Matrix (4x4) + умножение / transformPoint / translation / scaling
- [x] Камера: perspectiveFovRH / lookAtRH (для 3D-вьюпорта) — проверено
- [~] Quaternion
- [ ] BoundingBox / BoundingSphere / Ray / Plane

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
- [x] Meta (RSC структурированные метаданные: StructureInfos / EnumInfos / DataBlocks + root) — проверено
- [x] MetaTypes (типизированное чтение значений/массивов по схеме указателей блок+смещение) — проверено
- [ ] MetaNames (известные хеши имён структур/полей)
- [x] Pso (PSIN/PMAP секции, data-map, root) — проверено
- [x] PsoSchema (PSCH): структуры схемы + поля (тип/offset) — проверено
- [ ] PSO типизированное чтение значений по схеме
- [x] Rbf (бинарный XML: дерево узлов structure/uint/bool/float/float3/string) — проверено
- [ ] XmlMeta / XmlPso / XmlRbf (экспорт в XML)

### Этап 6 — Форматы файлов (FileTypes/*)
- [x] Ytd (текстуры): TextureDictionary / Texture / TextureData (legacy/non-gen9) — проверено
- [x] Ybn (коллизии): Bounds + BoundComposite (дерево) + BoundGeometry (квантованные вершины) — проверено
- [x] Ywr (waypoint record) / Yvr (vehicle record) — список записей; Ywr проверено
- [x] Ynd (path nodes): NodeDictionary + PathNode (40б) массив узлов — проверено
- [x] Ynv (nav mesh): заголовок NavMesh (bounds/counts/areaID) — проверено
- [x] Awc (аудио): заголовок AwcFile (magic/version/flags/streamCount/chunkIndices) — проверено
- [x] Mrf (move network 'MoVE'): заголовок + таблицы триггеров/флагов (имя→бит) — проверено
- [x] Rel (аудио данные): тип, data-блок, name table, индекс (hash/string), hash/pack таблицы — проверено
- [x] Ycd (ClipDictionary): структура-заголовок + массив указателей клипов — проверено
- [x] Ypt (ParticleEffectsList/ptxFxList): pgBase заголовок + имя + указатели суб-словарей — проверено
- [x] DistantLights (.dat, big-endian): сетка ночных огней HD/non-HD, узлы + пути — проверено
- [x] Watermap ('WMAP', big-endian): заголовок + comp-headers + реки/озёра/бассейны — проверено
- [x] CacheDat (cache_y*.dat): версия + fileDates + модуль BoundsStore (32б items) — проверено
- [x] Gtxd (CMapParentTxds через RBF): карта child→parent TXD — проверено
- [x] Gxt2 (глобальный текст: hash→строка) — проверено
- [x] Heightmap (.dat: размеры/bbox/min-max высоты, RLE) — проверено
- [x] Определение типа файла по расширению (`gamefile`, ~31 тип, +Mrf) — проверено
- [x] PSO типизированный доступ к значениям по блоку+смещению поля (big-endian) — проверено
- [x] PSO PSCH парсинг enum-определений (type==1: nameHash→ключи) — проверено
- [x] JenkIndex массовая индексация (`EnsureAll`) — проверено
- [x] RBF → XML экспорт (`rbf_xml`) — проверено
- [x] Meta → XML экспорт (`meta_xml`): структуры/скаляры/векторы/хеши/enum/массивы + имена через JenkIndex — проверено
- [x] PSO → XML экспорт (`pso_xml`): схемо-управляемая сериализация полей + enum-имена — проверено
- [x] XML-билдер общий (`xml_builder`): экранирование, отступы, теги — используется meta/pso xml
- [x] Эксплорер: превью Meta/PSO/RBF/Ymap/Ytyp с полным XML (`ExplorerModel::getXml`) — собирается
- [x] CLI: команды `xml` (дамп в XML) и `meta` (инспектор структур) — собирается
- [x] Yed/Yfd/Yld (Expression/FrameFilter/Cloth dictionaries): base + имена-хеши + счётчик items — проверено (Yfd)
- [x] Рендер .yft (FragType.drawable) и .ydd (drawable по индексу) — `buildFragment/DictionaryRenderModel`, собирается
- [x] GUI: рендер .yft/.ydd во вьюпорте + панель XML-дампа Meta/PSO/RBF с кнопкой «Save XML» — собирается (build-gui)
- [x] Исправлен краш «vector subscript out of range»: безопасное построение дерева RPF
      (беззнаковые индексы, clamp диапазонов, visited-защита от циклов) + проверка размера TOC — проверено
- [x] Навигационная модель эксплорера: виртуальное дерево папок/архивов из путей записей
      (`listChildren`/`isDirectory`), тип+размер записей — проверено
- [x] GUI 1:1 со стилем EvelentWalker/CodeWalker: меню (File/View/Help), тулбар (папка/Scan/Up/путь/поиск),
      левое дерево архивов + список с колонками Name/Type/Size + панель превью + статус-бар + About — собирается
- [x] Загрузка ключей GTA5 из папки `Keys` (CodeWalker-формат: aes/ng_key/decrypt_tables/hash_lut)
      — `GTA5Keys::loadFromKeysFolder` + автозагрузка в эксплорере/CLI, статус ключей в GUI — проверено
- [x] Исправлен повторный краш: NG/AES-расшифровка без ключей бросает ловимое исключение
      (вместо индексации пустых `PC_NG_KEYS`/`PC_LUT`); зашифрованный архив без ключей пропускается — проверено
- [x] SHA-1 (FIPS 180-1) — проверен по эталонным векторам ("", "abc", 56-байт)
- [x] HashSearch (поиск ключевых блоков по SHA-1, 8-байтное выравнивание, многопоточно) — проверено
- [x] Извлечение AES-ключа из `gta5.exe` по известному SHA-1 (`loadAesKeyFromExe`) + фолбэк в эксплорере/CLI — проверено
      (NG-ключи/таблицы пока требуют папку `Keys`/magic.dat — для них нужна полная таблица из 374 SHA-1 хешей)
- [x] Извлечение файлов на диск из эксплорера/GUI (`ExplorerModel::extractToFile` + кнопка Extract) — проверено
- [x] CLI: команда `tree` (листинг виртуальной папки с типами/иконками каталогов) — собирается
- [x] Ypdb (pose matcher database): заголовок (marker/versions/signature/samples) + тип файла — проверено
- [x] Heightmap → PGM экспорт (`HeightmapFile::toPGM`) — проверено
- [x] Fxc (компилированные шейдеры 'rgxe'): заголовок + vertexType + preset-параметры + тип файла — проверено
- [x] PSO → XML: рекурсия вложенных inline-структур (тип Structure) — проверено
- [x] Эксплорер: превью .fxc (vertexType + preset params) и .ypdb — собирается
- [x] Рендер: цвета вершин в `RenderMesh.colors` + `computeBounds` (AABB/центр/радиус для камеры) — проверено
- [x] Текстуры: декод L8/A8/A1R5G5B5 + BC4(ATI1)→grayscale, `textureFormatName` — проверено
- [x] JenkIndex: `LoadStringsFromFile` (вордлист для резолва хешей) — проверено
- [x] Heightmap: `toPGMMin` (мин-высоты в PGM) — проверено
- [x] CLI: команды `hash <string>` и `types <folder>` (счётчик типов записей) — собирается
- [x] GUI: рендер словаря .ydd с выбором индекса drawable, превью текстур .ytd (D3D11 SRV из RGBA, переключение текстур) — собирается
- [x] Ymap (карты мира): CMapData / CEntityDef через Meta + массив указателей — проверено
- [x] Ytyp (архетипы): CMapTypes / CBaseArchetypeDef — связь entity→архетип→drawable/texture — проверено
- [ ] gen9-вариант текстур (Enhanced)
- [~] Модели: Ydr/Ydd/Yft (Drawable/Fragment)
  - [x] Геометрия: VertexDeclaration / VertexBuffer / IndexBuffer (legacy) — проверено
  - [x] Декодирование вершин: типы/смещения компонентов + извлечение позиций/нормалей/UV/цветов — проверено
  - [x] DrawableGeometry / DrawableModel (сборка сеток: VB+IB+shaderId+AABB) — проверено
  - [x] ShaderGroup / ShaderFX (материалы + встроенный TextureDictionary) — проверено
  - [x] DrawableBase / Drawable (Ydr: ShaderGroup + LOD-списки моделей High/Med/Low/VLow) — проверено
  - [x] Skeleton (кости: трансформации + parent/child индексы; теги отложены) — проверено
  - [x] ShaderParametersBlock (параметры материала: текстуры + Vector4 + хеши) — проверено
  - [x] Ydd (DrawableDictionary: хеши + массив Drawable) — проверено
  - [x] Yft (FragType): заголовок фрагмента (bounding, gravity/buoyancy, glass count) + основной Drawable — проверено
  - [ ] Joints / Bounds (коллизия); Yft (Fragment)- [ ] Карты: Ymap/Ytyp
- [ ] Остальные ~35 форматов

### Этап 6.5 — Приложение
- [x] CLI `evw_cli` (scan / extract / ytd) поверх ядра — связывает все слои, собирается

### Этап 7 — Рендеринг
- [ ] Абстракция устройства Direct3D 11
- [ ] Шейдеры, Renderable, Renderer

### Этап 8 — UI (ImGui)
- [x] Выбор фреймворка: **Dear ImGui + Win32 + Direct3D 11** (как у CodeWalker — DX11,
      HLSL-шейдеры переиспользуемы; ImGui-бэкенды `imgui_impl_win32` + `imgui_impl_dx11`)
- [x] UI-agnostic модель эксплорера в ядре (`ExplorerModel`: scan/search/openFile) — проверено
- [x] ImGui RPF-эксплорер (`evw_gui`): поиск, список записей, типизированный превью — собирается
- [x] 3D-вьюпорт: D3D11 offscreen-рендер Drawable (HLSL VS/PS, орбитальная камера, освещение)
      → показ в ImGui как ImTextureID(SRV); вращение мышью, зум колесом — собирается
- [x] Текстурирование моделей: декод YTD-текстур (DXT→RGBA) → D3D11 SRV, привязка к мешам
      по материалу (ShaderParametersBlock); UV-семплинг в шейдере — собирается
- [ ] Перенос настоящих HLSL-шейдеров CodeWalker (вместо упрощённого)
- [ ] Полный набор экранов CodeWalker (карта мира, редакторы)

### Сборка GUI

```cmd
cd cpp
cmake -S . -B build-gui -DEVW_BUILD_GUI=ON
cmake --build build-gui --config Debug --target evw_gui
build-gui\Debug\evw_gui.exe "C:\путь\к\GTA V"
```
GUI тянет только Dear ImGui (FetchContent); d3d11/dxgi/d3dcompiler — из Windows SDK.
Основная сборка (`build.cmd`) GUI не требует — он за опцией `EVW_BUILD_GUI` (по умолчанию OFF).

### Почему DirectX 11, а не OpenGL
CodeWalker рендерит на D3D11 (SharpDX), шейдеры — HLSL (папка `Shaders/`). DX11 позволяет
переиспользовать пайплайн и шейдеры почти как есть; OpenGL потребовал бы переписывания в GLSL.
Цель — Windows (оригинал Windows-only), кроссплатформенность не нужна. ImGui имеет родной DX11-бэкенд.

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
    (VB+IB+shaderId+AABB), проверено на синтетической модели с одной геометрией;
  - **материалы** (`shader`): ShaderGroup / ShaderFX (+ встроенный TextureDictionary), проверено;
  - **Ydr-уровень** (`drawable`): DrawableBase / Drawable — ShaderGroup + LOD-списки моделей
    (High/Med/Low/VLow) + bounding, проверено на синтетическом drawable;
  - **скелет** (`Skeleton`): трансформации костей (матрицы) + parent/child индексы;
  - **параметры материалов** (`ShaderParametersBlock`): текстуры (ссылки на TextureBase) +
    Vector4-параметры + хеши имён — связывает шейдер с его текстурами, проверено;
  - **Ydd** (`DrawableDictionary`): словарь drawable по хешам имён, проверено;
  - **мета-контейнер** (`Meta`): RSC структурированные метаданные — определения структур
    (с записями полей), enum'ы, data-блоки + корневой блок; поиск структуры по хешу,
    проверено на синтетическом meta (структура+enum+data block+name);
  - **мета-чтение** (`metatypes`): типизированное чтение значений/массивов из data-блоков
    по схеме указателей (индекс блока + смещение), проверено (массив, смещение, единичное, по имени);
  - **Ymap** (`ymap`): CMapData / CEntityDef — размещение объектов мира через Meta + массив
    указателей на сущности (проверено на синтетическом ymap с двумя entity);
  - **Ytyp** (`ytyp`): CMapTypes / CBaseArchetypeDef — определения архетипов (drawable/texture
    словари по имени объекта), замыкает связь entity→архетип→модель, проверено;
  - **модель эксплорера** (`ExplorerModel`): scan папки, поиск, типизированный openFile
    (YTD/YDR/YDD/binary), проверено на временном `.rpf`;
  - **ImGui-фронтенд** (`evw_gui`): RPF-эксплорер на **Dear ImGui + Win32 + Direct3D 11**
    (бэкенды imgui_impl_win32/imgui_impl_dx11) — как у CodeWalker; собирается в `evw_gui.exe`;
  - **математика камеры**: умножение матриц, transformPoint, perspectiveFovRH/lookAtRH (проверено);
  - **сборка рендер-сеток** (`render_mesh`): из Drawable/Model → меши (позиции/нормали/UV/индексы +
    shaderId + AABB), готовые для загрузки в GPU; проверено;
  - **3D-вьюпорт** (`gui/viewport`): D3D11 offscreen-рендер Drawable (HLSL VS/PS, depth, орбитальная
    камера, диффузное освещение), вывод в ImGui как ImTextureID(SRV), вращение мышью/зум; собирается;
  - **текстурирование** (`buildRenderModel` + viewport): встроенные YTD-текстуры декодируются
    (DXT→RGBA) и грузятся в D3D11 SRV, привязка к мешам по материалу (ShaderParametersBlock),
    UV-семплинг в шейдере; собирается;
  - **коллизии** (`bounds`): базовый заголовок Bounds (тип, box/sphere, material, volume) — проверено;
  - **PSO** (`pso`): альтернативный мета-контейнер (big-endian секции PSIN/PMAP), data-map записи,
    root id, детект формата по magic; проверено на синтетическом PSO;
  - **RBF** (`rbf`): бинарный XML — дерево типизированных узлов (structure/uint/bool/float/float3/
    string/bytes), рекурсивный парсер; проверено на синтетическом дереве;
  - **RBF→XML** (`rbf_xml`), **Gxt2** (текст hash→строка), **Heightmap** (.dat), **записи**
    Ywr/Yvr (`records`), **BoundComposite** (дерево коллизий), **детект типа файла** (`gamefile`),
    **Quaternion→Matrix** — все собираются, ключевые проверены тестами;
  - ExplorerModel/openFile расширен превью YBN/GXT2/**YFT/YND/YNV/AWC** + детект PSO/RBF по magic.

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
