# PES-VCS Lab Report
**Name:** Atharva Anand  
**SRN:** PES1UG24CS092

## Phase 5: Branching and Checkout

**Q5.1:** To implement `pes checkout <branch>`:
- `.pes/HEAD` must be updated to `ref: refs/heads/<branch>`
- The working directory files must be updated to match the target branch's tree
- This is complex because: files present in current branch but not target must be deleted, 
  files in target but not current must be created, and modified files must be restored.
  If the user has uncommitted changes, checkout must refuse to avoid data loss.

**Q5.2:** To detect dirty working directory conflict:
- For each file in the current index, compare its stored hash against the target branch's tree hash for the same path
- Also stat() each file and compare mtime/size against the index entry
- If any tracked file differs between branches AND has local modifications (mtime/size mismatch with index), refuse checkout

**Q5.3:** Detached HEAD means HEAD contains a commit hash directly instead of `ref: refs/heads/main`.
- If you commit in this state, new commits are created but no branch pointer moves to track them
- They become unreachable once you switch away
- Recovery: run `pes branch new-branch` while still on that commit to create a branch pointing to it

## Phase 6: Garbage Collection

**Q6.1:** GC algorithm:
1. Start from all branch refs in `.pes/refs/heads/`
2. Walk each commit chain, collecting all reachable commit hashes
3. For each reachable commit, add its tree hash; recursively add all subtree and blob hashes
4. Use a hash set (e.g. a hash table or sorted array) to track all reachable ObjectIDs
5. Walk all files in `.pes/objects/` — delete any whose hash is NOT in the reachable set
- Data structure: a hash set of 32-byte ObjectIDs for O(1) lookup
- For 100k commits, 50 branches: visit ~100k commits + ~100k trees + ~500k blobs ≈ 700k objects

**Q6.2:** Race condition: GC marks an object as unreachable, then a concurrent `pes add` writes 
a new blob but hasn't yet linked it into a commit. GC deletes the blob before the commit 
references it, causing corruption.
- Git avoids this by: (1) using a grace period — objects newer than 2 weeks are never deleted,
  (2) writing the new object before updating any refs, so the object exists before it's reachable
