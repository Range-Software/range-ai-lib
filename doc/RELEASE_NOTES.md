## Version 1.2.0

### Improvements

#### RAgentFileStore

- The remote file listing is re-used for 5 minutes (`remoteFilesCacheTtlSecs`)
  and kept up to date with the store's own uploads, replacements and evictions.
  A query on an already uploaded file now needs no request to the Files API,
  and concurrent queries no longer wait for each other behind a listing request
- Any failed operation drops the cached listing, so the next call checks the
  remote store again
- New `invalidateRemoteFileListing()` drops the cached listing on request, for
  example after a request referencing an uploaded file has failed because the
  file was removed from the remote store by someone else. It never blocks, even
  while an upload is in progress
- Files removed from the remote store by someone else are noticed only after
  the listing expires or is invalidated

---

## Version 1.1.0

### Improvements

- Added `RAgentFileStore`: an Anthropic Files API client (upload, list, delete) with a persisted local index that re-uses an unchanged file by content hash, replaces a changed one, and evicts least-recently-used files when the store capacity would be exceeded
- `RAgent::chat()` can attach a file already uploaded to the Anthropic Files API, referencing it as a document or image block (Anthropic type only) and adding the required beta header
- Default Anthropic model updated to `claude-sonnet-5`

---

## Version 1.0.0

Initial release.
