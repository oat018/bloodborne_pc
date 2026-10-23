# CL01 progress — external Claude Code owner

Status: VERIFIED (2026-10-23). The documents and their checks are complete. This status does not qualify any game feature. Parent: R01/R04 and W03/W06. Owner: external Claude Code collaborator (not started by Codex).

Source pins:
- BBport `e5d04e2ba4864ccf40de6c3cf6aa532855320b0d`
- BBSC `4f11faf88b6ba658ebde6fe5fbefdadedba4fa41`
- Also read: shadnet-p2p `5a849a192fd4450593c57b10697316ac324c856e`, the source of the release's `shadnet.exe` according to the BBSC server README.

Read the [work package](../../handoff/CLAUDE-CODE-WINDOWS.md). Write ownership is restricted to the four files listed there. This file is the child tracker; Codex mirrors meaningful outcomes to the central tracker.

| Slice | Status | Evidence / next action |
|---|---|---|
| CL01-A reference behavior/source map | VERIFIED | [BBSC-002-MAP.md](BBSC-002-MAP.md): actual load path, composition facts C1–C6 and the three-goal matrix. Next: Codex uses it to assemble R02. |
| CL01-B identity/protocol trust chain | VERIFIED | [IDENTITY-PROTOCOL.md](IDENTITY-PROTOCOL.md): self-host route (every port, one address, no public dependency), identity chain, findings F1–F5. Next: R03 reproduces F1 and F3. |
| CL01-C acceptance cases + handback | VERIFIED | [ACCEPTANCE-CASES.md](ACCEPTANCE-CASES.md): 23 cases (A-J1..7, P-1..9, S-1..4, R-1..3) and a suggested R03 order. None has been run. |

Checks: Markdown link/anchor check, UTF-8 decode, LF/no-BOM and `git diff --check`, all clean on the committed documents. No feature, native or gameplay acceptance is claimed.

## Checkpoint 2026-10-23

- **Branch:** `claude/bbsc002-reference-audit`, from origin/windows `15bbdec`. The commit is local and not pushed. Publishing needs the human's OK.
- **Changed paths:** only the four allowlisted files in `docs/collaboration/claude/`.
- **Inputs:**
  - Read-only reference clones in the ignored `.superpowers/references/`.
  - The release assets `BBSC-0.0.2-BBport.zip` (SHA256 `001efc0a…cd3c`) and `BBSC-0.0.2-Server.zip` (SHA256 `54b0bd1b…6213`). Only file lists and plaintext scripts were read. No binary was executed.
- **Patch digest:** the Git blob `e34c79b3…2aa3` matches the plan. The copy in the release zip is CRLF with a BOM, so its SHA256 is `84edf6d3…e7d5`.
- **Not run:** builds, the game, servers, accounts, native or gameplay tests. No private endpoints, accounts or logs were used.

## Top findings (details and citations in the documents)

1. **F1, G1 SOURCE-GAP.** In seamless mode, the shadNet broker matches a guest's sign across maps whenever the host's password is empty, so a guest with any password can be pulled cross-map. The client never checks `SummonWord`. Fix options: (a) change the server's `MatchesSearch`, or (b) add a client-only check on the aimed placement.
2. **F3, G2 SOURCE-GAP.** Shared progress collects the host's whole boss-flag set and all kind-1 groups of every block snapshot (`WORLD_SYNC_ALL` is on by default). There is no admission baseline, so the host's pre-party history is imported. Smallest delta: a first-seen baseline inside the existing collector.
3. **C3/C4, composition.** BBSC finds saves only under `<cwd>\user`, `%APPDATA%\shadPS4` or `BBSC_SAVE_DIR`, and keeps its journals next to the DLL. Copied profiles therefore need `BBSC_SAVE_DIR` and a separate package copy per client.
4. **C2, startup.** The unchanged 1.7.2 loader can race the DLL's watcher thread and fault at `bbsc.dll+0x1933`. The upstream fix, Supermedo#125, is open.
5. **G3.** The release already contains a self-hosted route: shadNet on 31313/31314/31315 and BBSC-GameServer on 20443/20444 plus UDP 3478, all on one address. It needs configuration only.

Unverified assumptions:
- The release binaries are built from the stated sources.
- `BBSC-GameServer.exe` is a packaged `bbsc_gameserver.py`.
- Kind-1 groups hold no NPC quest-phase bits.

## Requested changes outside the allowlist

None. Proposed runtime and server changes are recorded only as findings in the owned documents.
