// Sorry that the Documentation is written in a C file

// ───────────────────────────────────────────────────────────────
// OVM File Structure
// ───────────────────────────────────────────────────────────────
//
// ┌────────┬─────────┬────────────┬────────┬────────────────────┐
// │ MAGIC  │ VERSION │ FLAGS_SIZE │ FLAGS  │       TOKENS       │
// └────────┴─────────┴────────────┴────────┴────────────────────┘
//
// MAGIC        : 4 bytes       Indicates the file as an OVM binary
// VERSION      : 1 byte        Indicates OVM File Format Version
// FLAGS_SIZE   : 1 byte        Indicates how many flags has the OVM binary
// FLAGS        : 4 bytes       Contains all flags the OVM binary has
// TOKENS       : Remaining     Contains all tokens

// Each token is 10 bytes  | NOTE: if a token contains a string it can be bigger than 10 bytes
// ───────────────────────────────────────────────────────────────

