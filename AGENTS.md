# Working in vkEngine

## Coding standards

- Match the naming and code style of the existing code. In particular, non-public C++ member functions must use lower camel case (`lowerCamelCase`).
- Consider algorithmic complexity when writing code, especially searches that enumerate a collection. Check whether an existing lookup structure can answer the query directly. Small, bounded scans are acceptable; do not add a data structure solely to eliminate such a scan.
- Before adding checks or defensive code, examine where the operation sits in the full flow and which invariants its callers or earlier stages have already established. Avoid duplicate validation and checks for errors that are logically impossible at that point. Validate external input at the appropriate boundary and preserve the established invariants downstream.
- Prefer a direct implementation using existing project abstractions. Introduce helpers or abstractions when they solve a concrete problem in the task.
- Before finishing, review the changed code in its calling context for redundant validation, avoidable scans, duplicated state, unnecessary conversions, and unused code.

## Documentation and comments

Technical documentation describes the current implementation and its public contracts. Write for a reader who has not seen the development conversation.

- Do not modify any files under `docs/devlog/`.
- Do not modify `AGENTS.md` unless the user explicitly requests it.
- When a design changes, rewrite the affected explanation around the resulting design. Remove stale names, superseded mechanisms, and duplicate explanations.
- Avoid defining the current design by denying an abandoned or hypothetical alternative. Phrases such as "no longer," "instead of the old implementation," "removed X," and "does not use Y" usually belong in change history.
- Keep negative statements when they express a meaningful contract: invalid inputs, unsupported operations, pointer lifetime restrictions, or observable failure and cleanup behavior. Explain the consequence for the caller.
- Keep implementation descriptions separate from future work. Label unfinished features explicitly and avoid presenting planned behavior as available.
- Preserve the language of the document being edited. Use exact API names and verify examples, links, call order, and return types.
- Comments should explain intent, invariants, or non-obvious constraints. Avoid narrating the edit or repeating the surrounding code.

## Build

See [readme.md](readme.md) for build instructions.

## Testing

- You may write and run tests while modifying code. Unless the user explicitly requests that they be retained, remove all test-related content you created during the task before finishing development. This includes test code, scripts, fixtures, configurations, generated outputs, and temporary test build integration. Preserve pre-existing tests and user-authored changes.
