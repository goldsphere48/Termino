  Архитектура: три прохода + единая таблица символов

  Семантический анализатор удобно построить как три последовательных прохода по уже готовому ProgramNode. Результат —
  модифицированный AST (с подставленными константами и адресами) + SymbolTable для кодогенератора.

  ProgramNode (AST)
        │
        ▼
  [Pass 1] collectSymbols      ── строит SymbolTable (equ + data labels + code labels), ловит дубликаты
        │
        ▼
  [Pass 2] resolveAndValidate  ── подставляет equ, сворачивает const-выражения, проверяет инструкции/типы/overflow
        │
        ▼
  [Pass 3] computeLayout       ── раскладывает .data и .code по адресам, резолвит ссылки на метки в адреса
        │
        ▼
  готовый AST + SymbolTable → codegen

  Что нужно добавить в isa.h/isa_helper.cpp ПЕРЕД анализатором

  Сейчас в коде нет спецификации инструкций — без этого Pass 2 писать нельзя. Добавь таблицу/функцию:

  enum class OperandKind { I8, I16, I32, F32, DATA_LABEL, CODE_LABEL };

  struct InstructionSpec
  {
      std::vector<OperandKind> operands;   // PUSH8 -> {I8}, ADD -> {}, JMP -> {CODE_LABEL}, LOAD -> {DATA_LABEL}
  };

  const InstructionSpec& getInstructionSpec(OP_CODE op);

  Это единая точка правды — Pass 2 проверяет instr.operands.size() против spec.operands.size() и тип каждого операнда.

  Скелет semantic.h

  #pragma once
  #include "parser.h"
  #include "error.h"
  #include <unordered_map>
  #include <variant>
  #include <cstdint>

  enum class SymbolKind { EQU, DATA, CODE };

  struct Symbol
  {
      SymbolKind kind;
      std::variant<int, float, uint32_t> value;  // EQU: число; DATA/CODE: адрес
      size_t line = 0, column = 0;
  };

  class SemanticAnalyzer
  {
  public:
      SemanticAnalyzer(ProgramNode& program, ErrorCollector& errors);
      void analyze();
      const std::unordered_map<std::string, Symbol>& symbols() const { return m_symbols; }

  private:
      void collectSymbols();
      void resolveAndValidate();
      void computeLayout();

      // возвращает свернутое значение, если выражение целиком константное
      std::optional<std::variant<int,float>> foldExpression(ExpressionNode* expr);
      void validateInstruction(InstructionNode& instr);
      size_t dataItemSize(const DataNode& d) const;     // .byte = 1, .short = 2, ...

      ProgramNode& m_program;
      ErrorCollector& m_errors;
      std::unordered_map<std::string, Symbol> m_symbols;
  };

  Что делает каждый проход — детально

  Pass 1 — collectSymbols
  - Идешь по m_program.equSection → добавляешь в таблицу (дубликаты в самой equ уже ловит парсер, но межсекционные
  коллизии — нет: counter: в .data и counter в .equ должны быть ошибкой).
  - Идешь по DataSectionNode::datas → каждое DataNode::label в таблицу с kind=DATA (адрес пока 0).
  - Идешь по CodeSectionNode::nodes, ищешь LabelDefNode → в таблицу с kind=CODE.
  - Адреса оставляешь нулевыми — это работа Pass 3.

  Pass 2 — resolveAndValidate (главный по объему)
  - Для каждого InstructionNode:
    - Достаешь InstructionSpec. Если operands.size() не совпадает — ошибка WRONG_OPERAND_COUNT.
    - Для каждого операнда вызываешь foldExpression. Внутри рекурсивно: если IdentifierNode и это EQU — подставляешь
  значение; если BinaryOperationNode — складываешь левую и правую части (обе должны быть числами после fold).
    - Если ожидался I8/I16/... и после fold получилось число — проверяешь диапазон (для PUSH8 0..255 или -128..127).
    - Если ожидался DATA_LABEL/CODE_LABEL — операнд должен остаться IdentifierNode, и идентификатор должен быть в
  таблице с нужным kind. Резолв в адрес — это уже Pass 3.
  - Для DataNode: каждое выражение фолдишь и проверяешь, что влезает в ширину директивы (.byte → 0..255 и т.д.).

  Ключевой момент: после Pass 2 в AST не должно остаться IdentifierNode, указывающих на EQU, и не должно остаться
  BinaryOperationNode. Только числа или ссылки на метки.

  Pass 3 — computeLayout
  - dataAddr = 0. Идешь по DataSectionNode::datas: записываешь dataAddr в m_symbols[label].value, прибавляешь
  dataItemSize(d) * count.
  - codeAddr = 0. Идешь по CodeSectionNode::nodes: для LabelDefNode пишешь codeAddr в таблицу; для InstructionNode
  прибавляешь её закодированный размер (это знание тоже должно жить в InstructionSpec — 1 байт опкод + сумма размеров
  операндов).
  - Второй проход: заменяешь оставшиеся IdentifierNode на NumberNode со значением из таблицы. После этого кодогенератор
  работает только с числами.

  Новые ErrorType для error.h

    - Достаешь InstructionSpec. Если operands.size() не совпадает — ошибка WRONG_OPERAND_COUNT.
    - Для каждого операнда вызываешь foldExpression. Внутри рекурсивно: если IdentifierNode и это EQU — подставляешь значение; если BinaryOperationNode — складываешь левую и правую части
  (обе должны быть числами после fold).
    - Если ожидался I8/I16/... и после fold получилось число — проверяешь диапазон (для PUSH8 0..255 или -128..127).
    - Если ожидался DATA_LABEL/CODE_LABEL — операнд должен остаться IdentifierNode, и идентификатор должен быть в таблице с нужным kind. Резолв в адрес — это уже Pass 3.
  - Для DataNode: каждое выражение фолдишь и проверяешь, что влезает в ширину директивы (.byte → 0..255 и т.д.).

  Ключевой момент: после Pass 2 в AST не должно остаться IdentifierNode, указывающих на EQU, и не должно остаться BinaryOperationNode. Только числа или ссылки на метки.

  Pass 3 — computeLayout
  - dataAddr = 0. Идешь по DataSectionNode::datas: записываешь dataAddr в m_symbols[label].value, прибавляешь dataItemSize(d) * count.
  - codeAddr = 0. Идешь по CodeSectionNode::nodes: для LabelDefNode пишешь codeAddr в таблицу; для InstructionNode прибавляешь её закодированный размер (это знание тоже должно жить в
  InstructionSpec — 1 байт опкод + сумма размеров операндов).
  - Второй проход: заменяешь оставшиеся IdentifierNode на NumberNode со значением из таблицы. После этого кодогенератор работает только с числами.

  Новые ErrorType для error.h

  UNDEFINED_SYMBOL
  DUPLICATE_SYMBOL_CROSS_SECTION
  WRONG_OPERAND_COUNT
  WRONG_OPERAND_TYPE
  VALUE_OUT_OF_RANGE        // для .byte 300, push8 1000 и т.п.
  DIVISION_BY_ZERO          // если добавишь / в const-выражения
  NON_CONSTANT_EXPRESSION   // если в .equ или data попал label

  Пара тонких мест, на которые сразу заложиться

  1. Циклы в EQU (A = B + 1, B = A * 2) — если разрешишь ссылки между equ, нужен detection через DFS с цветовой маркировкой. Проще пока запретить: equ принимает только числовые литералы и
  арифметику над ними.
  2. Forward references меток в .code — пользователь может сделать jmp end до того, как end: объявлена. Поэтому Pass 1 обязан собрать ВСЕ метки до Pass 2.
  3. PUSH8 -1 — решить, знаковая ли это 8-битка. Из текущего ISA не следует. Это определит границы overflow-проверки.
  4. main.cpp — после парсера добавить:
  SemanticAnalyzer sema(ast, errors);
  sema.analyze();
  if (errors.hasError()) { errors.printAll(); return 1; }

  С чего конкретно начать — с InstructionSpec и Pass 1. Без спецификации инструкций остальное не имеет смысла; Pass 1 без неё уже даст рабочую таблицу символов, на которой можно постепенно
   нарастить Pass 2 и .3

---

## Прогресс (на 2026-05-30)

СДЕЛАНО и работает:
- **Pass 1 — collectSymbols**: собирает EQU + DATA + CODE символы в `m_symbols`, ловит
  межсекционные дубликаты (`SYMBOL_REDEFINITION`).
- **Pass 2, половина 1 — свёртка EQU-констант**: `resolveAndValidate` идёт по EQU-символам и
  зовёт `resolveSymbol`. Реализованы:
  - `foldExpression(const ExpressionNode*)` → `optional<FoldedValue>` (рекурсия по
    NumberNode / IdentifierNode / BinaryOperationNode).
  - `resolveSymbol` с трёхцветной маркировкой (`RESOLVE_STATE` UNVISITED/IN_PROGRESS/DONE):
    мемоизация, детекция циклов (`CYCLED_DEPENDECIE`), forward-ссылки между EQU.
  - Хелперы: `ApplyUnariOperator`, `ApplyBinariOperator` (продвижение типов через
    `std::common_type_t`), `ToSymbolValue`/`FromSymbolValue`.
  - Типы: `FoldedValue = variant<int,float>` (домен свёртки),
    `SymbolValue = variant<int,float,Address>` (хранилище: EQU→число, лейбл→адрес из Pass 3).
  - Гард `kind != EQU` в ветке IdentifierNode (ссылка на лейбл в const-выражении → nullopt,
    это работа Pass 3).
- `printDeclaryedSymbols` печатает реальные значения через `std::visit`.

## СЛЕДУЮЩИЙ ШАГ — Pass 2, половина 2: валидация инструкций и данных

Сигнал: таблица `m_instructionSpecs` построена в конструкторе, но её пока никто не читает.
`resolveAndValidate` сейчас трогает только EQU.

Подзадачи:
1. `void validateInstruction(const InstructionNode&)`:
   - достать `InstructionSpec` по `command` (нет в таблице → решить трактовку инструкций без
     операндов);
   - `operands.size()` != `spec.operands.size()` → `WRONG_OPERAND_COUNT`;
   - по каждому операнду в зависимости от `OPERAND_KIND`:
     - `I8/I16/F16/U32` → `foldExpression` → должно свернуться в число → range-check через
       `fitsInKind` → иначе `VALUE_OUT_OF_RANGE`;
     - `CODE_LABEL` → операнд обязан остаться IdentifierNode, символ существует и `kind == CODE`
       (`WRONG_OPERAND_TYPE` иначе); резолв в адрес — Pass 3.
2. `void validateData(const DataNode&)`: свернуть каждое выражение, проверить ширину директивы
   (`.byte` → 0..255 и т.д.).
3. `bool fitsInKind(FoldedValue, OPERAND_KIND)` — диапазоны. ЗАОДНО решить открытый вопрос:
   знаковая ли `I8` (`-128..127` vs `0..255`) — см. «тонкие места» п.3 (PUSH8 -1).
4. Новые `ERROR_TYPE` (проверить, чего ещё нет в `error.h`): `WRONG_OPERAND_COUNT`,
   `VALUE_OUT_OF_RANGE`, `WRONG_OPERAND_TYPE`.
5. Вызвать обходы `.code`/`.data` в `resolveAndValidate` после цикла по EQU.

Перед стартом: глянуть `error.h` (какие ERROR_TYPE уже есть) и поля `InstructionNode`/`DataNode`
в `parser.h`, чтобы писать под реальную структуру.

После этого Pass 2 закрыт целиком → остаётся **Pass 3 computeLayout** (адреса лейблов + замена
оставшихся IdentifierNode на адреса) → кодоген.

## Известные мелочи / долги
- После провала свёртки (цикл/ошибка) `state=DONE`, но `value` остаётся дефолтным `int{0}` —
  повторный запрос вернёт `0`, а не `nullopt`. Спасает то, что `hasError()` прерывает компиляцию.
  Если понадобится честно — отдельный флаг `bool resolved` или хранить EQU в `optional<FoldedValue>`.
- DATA/CODE-лейблы в `printDeclaryedSymbols` печатают `0` до реализации Pass 3.
