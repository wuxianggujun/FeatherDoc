# FeatherDoc v1.13.3 public-header snapshot

The `snapshot` directory is an exact copy of the public headers from the
`v1.13.3` Git tag, plus that release's public pugixml headers. The ABI runtime
consumer must compile against this frozen tree and link only to the current
`FeatherDoc` target.

The snapshot deliberately remains unchanged when current public headers are
extended. Replace it only when introducing a new ABI compatibility baseline,
and always copy the files from the corresponding immutable release tag.
