## Version 1.1.0

### Improvements

- Added `RAgentFileStore`: an Anthropic Files API client (upload, list, delete) with a persisted local index that re-uses an unchanged file by content hash, replaces a changed one, and evicts least-recently-used files when the store capacity would be exceeded
- `RAgent::chat()` can attach a file already uploaded to the Anthropic Files API, referencing it as a document or image block (Anthropic type only) and adding the required beta header
- Default Anthropic model updated to `claude-sonnet-5`

---

## Version 1.0.0

Initial release.
