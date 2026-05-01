# Range AI Library

Qt-based C++ library for integrating LLM-backed AI agents into Range applications. Handles API communication, conversation state, and retry logic across multiple providers.

## What it provides

**Agent** — `RAgent` manages a full chat session: sends messages, maintains conversation history, issues asynchronous HTTP requests, and exposes signals (`responseReceived`, `errorOccurred`, `requestCancelled`) for Qt-based UI integration.

**Configuration** — `RAgentSettings` holds all per-agent parameters: provider type, API key and URL, model name, system prompt, temperature, max tokens, request timeout, proxy, and retry count. Supports JSON serialisation via `fromJson()`/`toJson()`.

**Settings management** — `RAgentSettingsManager` provides persistent CRUD storage for named `RAgentSettings` configurations.

## Supported providers

- OpenAI (chat completions API)
- OpenAI Legacy (completions API)
- Anthropic (Messages API)

## Class prefix

All public classes use the `R` prefix (e.g. `RAgent`, `RAgentSettings`).

## Dependencies

- Qt 6.10+
- range-base-lib
- range-cloud-lib
- C++17
