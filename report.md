# PES-VCS Lab Report
**Name:** Atharva Anand  
**SRN:** PES1UG24CS092

---

## Phase 1: Object Storage

**Screenshot 1A** — test_objects output:
<img width="1509" height="420" alt="Screenshot 2026-04-20 at 9 13 22 AM" src="https://github.com/user-attachments/assets/4722e45a-57a6-4087-8796-98827ae67785" />


**Screenshot 1B** — sharded object directory:
<img width="1503" height="648" alt="Screenshot 2026-04-20 at 9 14 25 AM" src="https://github.com/user-attachments/assets/aa78057e-c365-4ff1-ba31-d37959880584" />

---

## Phase 2: Tree Objects

**Screenshot 2A** — test_tree output:
<img width="1510" height="249" alt="Screenshot 2026-04-20 at 9 35 56 AM" src="https://github.com/user-attachments/assets/3ded1b41-5200-49b2-b2b4-ec7a56d93b13" />


**Screenshot 2B** — raw binary tree object:
<img width="1510" height="340" alt="Screenshot 2026-04-20 at 10 01 31 AM" src="https://github.com/user-attachments/assets/e6272c28-bc29-44ad-a0ba-7bb8683ef5da" />

---

## Phase 3: Index / Staging Area

**Screenshot 3A** — pes init → add → status:
<img width="1512" height="982" alt="Screenshot 2026-04-20 at 10 02 06 AM" src="https://github.com/user-attachments/assets/1b9465d2-a5f4-4527-abea-2317638fa3fa" />


**Screenshot 3B** — cat .pes/index:
<img width="1029" height="99" alt="Screenshot 2026-04-20 at 10 02 27 AM" src="https://github.com/user-attachments/assets/9f5c79be-2ab1-4324-9e62-546808629d50" />


---

## Phase 4: Commits and History

**Screenshot 4A** — pes log with 3 commits:
<img width="1508" height="694" alt="Screenshot 2026-04-20 at 10 06 17 AM" src="https://github.com/user-attachments/assets/8930385c-5994-43a3-a66c-1bbc29a689c7" />


**Screenshot 4B** — find .pes -type f | sort:
<img width="877" height="304" alt="Screenshot 2026-04-20 at 10 06 35 AM" src="https://github.com/user-attachments/assets/e88afd74-6176-43aa-be52-0f7804d2dabf" />


**Screenshot 4C** — HEAD and branch ref:
<img width="844" height="112" alt="Screenshot 2026-04-20 at 10 06 54 AM" src="https://github.com/user-attachments/assets/299408c7-533c-42cd-8c6e-53b7cefee6be" />


**Final Integration Test:**
<img width="1025" height="859" alt="Screenshot 2026-04-20 at 10 07 59 AM" src="https://github.com/user-attachments/assets/34636626-5cb9-4b1f-8243-f12e9f750906" />


---

## Phase 5: Branching and Checkout Analysis

**Q5.1:** To implement `pes checkout <branch>`, two things must change in `.pes/`:
HEAD must be updated to point to the new branch name, and the working directory
files must be replaced with the files from the target branch's tree object.
This is complex because files that exist in the current branch but not the target
must be deleted, files in the target but not current must be created, and modified
files must be overwritten. If the user has local modifications to any file that
differs between branches, checkout must refuse to prevent data loss.

**Q5.2:** To detect a dirty working directory conflict using only the index and object store:
For each file tracked in the current index, stat() the file on disk and compare
mtime and size against the stored index values. If they differ, the file has local
modifications. Then check if that same file path has a different blob hash in the
target branch's tree. If both conditions are true — locally modified AND different
between branches — refuse checkout.

**Q5.3:** Detached HEAD means HEAD contains a raw commit hash instead of
`ref: refs/heads/main`. If you make commits in this state, new commits are created
and HEAD advances, but no branch pointer tracks them. Once you switch away, those
commits become unreachable and will eventually be garbage collected.
Recovery: while still on the detached commit, create a new branch pointing to it
with `pes branch recovery-branch`, which saves the commit hash into a ref file.

---

## Phase 6: Garbage Collection

**Q6.1:** GC algorithm to find and delete unreachable objects:
1. Collect all branch refs from `.pes/refs/heads/` as starting points
2. Walk each commit chain, adding every commit hash to a reachable set
3. For each reachable commit, add its tree hash, then recursively add all
   subtree hashes and blob hashes reachable from that tree
4. Walk all files under `.pes/objects/` and delete any whose hash is NOT
   in the reachable set
Data structure: a hash set (hash table keyed by ObjectID) for O(1) lookup.
For 100k commits and 50 branches: roughly 100k commits + 100k trees +
500k blobs = ~700k objects to visit in the reachability pass.

**Q6.2:** Race condition between GC and commit:
1. GC scans all refs and marks reachable objects
2. `pes add` writes a new blob to the object store (not yet in any commit)
3. GC deletes the blob because it was not reachable at scan time
4. `pes commit` tries to reference the deleted blob — corruption occurs

Git avoids this by enforcing a grace period: any object newer than 2 weeks
is never deleted regardless of reachability. Additionally, Git writes new
objects before updating any refs, so the object always exists before it is
referenced. The grace period covers the window between object creation and
the ref update that makes it reachable.
