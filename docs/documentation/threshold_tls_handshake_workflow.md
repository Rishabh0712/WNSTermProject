# TLS 1.3 Handshake Workflow with Fast Multiparty Threshold ECDSA

This document gives a granular, step-by-step illustration of how a standard TLS 1.3 handshake proceeds when the server's ECDSA private key is managed via a Fast Multiparty Threshold ECDSA scheme with a prior Fast Trustless Distributed Key Generation (DKG).

---
## Legend
- C: Client
- FE: TLS Frontend (web server / reverse proxy integrating threshold signing engine)
- COORD: Threshold signing coordinator (stateless orchestrator)
- S_i: Signer node i holding a private share
- PK: Server certificate public key (from threshold DKG)
- PrePool: Precomputed nonce/share auxiliary data pool maintained by signers
- (r,s): Final ECDSA signature

---
## Pre-Handshake Preparation (Out-of-Band)
1. DKG Completed: All signer nodes S_1..S_N have run trustless DKG; each stores a share x_i; public key PK published; certificate issued.
2. Membership Established: Coordinator knows active signer list and quorum threshold T.
3. Precomputation Pool Filled: Each signer maintains a pool of precomputed ephemeral values (nonce k_i, R_i = k_i*G, inverses or auxiliary structures) registered with COORD.
4. Health Checks: FE periodically queries COORD for quorum readiness (e.g., PrePool depth ≥ MIN_POOL, at least T signers healthy).
5. Secure Channels: Mutual-auth TLS (bootstrap certs) or mTLS + ACL between FE ↔ COORD and COORD ↔ S_i.

---
## High-Level Phase Mapping
Standard TLS 1.3 phases:
A. ClientHello
B. ServerHello + KeyShare negotiation
C. EncryptedExtensions
D. Certificate
E. CertificateVerify (ECDSA signature over transcript) ← Threshold integration point
F. Finished (both sides)
G. Application Data

Threshold ECDSA only directly affects Phase E; other phases remain wire-identical.

---
## Step-by-Step Handshake Sequence (Happy Path)
1. C → FE: Sends ClientHello (supported groups, cipher suites, key shares, extensions).
2. FE: Selects cipher suite & key share parameters (e.g., X25519 or secp256r1). No threshold involvement yet.
3. FE → C: Sends ServerHello.
4. Key Schedule Derivation: FE computes handshake secrets from key exchange shared secret (HKDF steps per RFC 8446).
5. FE → C: Sends EncryptedExtensions.
6. FE → C: Sends Certificate (contains PK associated with threshold key; private half is never materialized).
7. FE: Prepares transcript hash TH_1 = Hash(all handshake messages up to Certificate).
8. FE: Requests threshold signature via COORD: `SignRequest { transcript_hash: TH_1, context: "TLS-1.3-CertificateVerify", nonce_policy: FRESH }`.
9. COORD: Selects a quorum Q of T healthy signers with available precompute entries.
10. COORD → Each S_i in Q: Issues SigningRoundStart referencing a chosen precompute tuple (nonce commitment R_i) or instructs creation if pool low.
11. Each S_i: Retrieves (or generates) precomputed (k_i, R_i, aux_i). Computes its partial s_i = k_i^{-1} * (TH_1 + r * x_i) mod n (or variant-specific formula). Returns (R_i, s_i, proof_i) to COORD.
12. COORD: Verifies consistency:
    - Aggregates R = Σ R_i; derives r = x_coord(R) mod n.
    - Validates each proof_i / sanity checks (e.g., s_i within [1,n-1]).
    - Computes final s = Σ s_i mod n; applies low-s normalization.
13. COORD → FE: Returns signature (r,s), quorum metadata (IDs, precompute IDs), timestamp.
14. FE: Forms CertificateVerify message with signature (r,s) exactly as if locally computed.
15. FE → C: Sends CertificateVerify.
16. FE: Computes Finished MAC over transcript including CertificateVerify; sends Finished.
17. C: Verifies ECDSA signature using PK (standard path, unaware of threshold process). If valid, proceeds.
18. C → FE: Sends its Finished message.
19. Secure Channel Established: Application data encrypted with derived traffic keys proceeds bidirectionally.

---
## Timing & Concurrency Notes
- Steps 8–13 (threshold signing) run concurrently with FE preparing subsequent data; FE may block sending CertificateVerify until signature arrives.
- Precomputation ensures step 11 partial calculation is constant-time and fast (< 2 ms typical).
- Coordinator parallelizes collection; network round trips kept intra-datacenter for minimal latency.

---
## Failure & Alternate Paths
### A. Insufficient Quorum
1. COORD cannot acquire T signers (offline nodes). Returns error `QuorumInsufficient`.
2. FE policy options:
   - Retry after backoff (e.g., 50 ms) if pool is refilling.
   - Serve 5xx / close connection gracefully.
   - Optional fallback to secondary traditional key (only if allowed by policy; reduces security).

### B. Precompute Pool Depleted
1. COORD responds `PrecomputeDepleted`.
2. FE triggers immediate refill request; COORD instructs signers to generate batch; FE retries when pool ≥ threshold.

### C. Malicious/Invalid Partial
1. COORD detects invalid s_i or inconsistent R_i.
2. Initiates blame protocol: requests commitment proofs from offender.
3. If confirmed malicious: signer quarantined, new signer selected; signing round restarts (returns transient error to FE if delay exceeds budget).

### D. Coordinator Failure Mid-Round
1. FE times out (e.g., > latency SLO 25 ms).
2. FE resubmits request to a different coordinator instance (idempotent by transcript hash).
3. Signers can reuse same precompute entries (unless nonce safety policy demands discard).

### E. Client Aborts Early
- Abort does not reveal private material; any partially used precompute entries are marked consumed to avoid nonce reuse.

---
## Security Checks During Signing
- Nonce Reuse Prevention: Each precompute tuple marked single-use at COORD; signers cross-check identifiers.
- Low-s Enforcement: Final s adjusted if high (s > n/2) to mitigate malleability.
- Transcript Binding: Hash TH_1 must match locally recomputed hash at signers (optional extra defense: COORD distributes TH_1 to signers for verification).
- Auditable Log: FE logs request ID + (r,s) + quorum IDs; COORD logs partials and aggregated R hash.

---
## Post-Handshake Considerations
- Session Resumption (Tickets): Unaffected (symmetric keys). Threshold signing reuse only if OCSP/SCT signing needed.
- Key Rotation: Dual-key period: FE may hold mapping of PK_old & PK_new; determines which threshold key signs based on certificate chain presented.
- Share Refresh: Occurs asynchronously; does not block handshakes; precompute pool regenerated afterward.

---
## Mermaid Sequence Diagram (Conceptual)
```mermaid
sequenceDiagram
    participant C as Client
    participant FE as TLS Frontend
    participant COORD as Coordinator
    participant S1 as Signer 1
    participant S2 as Signer 2
    participant S3 as Signer 3
    participant S4 as Signer 4

    C->>FE: ClientHello
    FE->>C: ServerHello + EncryptedExtensions + Certificate
    FE->>COORD: SignRequest(TH_1)
    COORD->>S1: PartialSign(TH_1)
    COORD->>S2: PartialSign(TH_1)
    COORD->>S3: PartialSign(TH_1)
    COORD->>S4: PartialSign(TH_1)
    S1-->>COORD: (R1, s1)
    S2-->>COORD: (R2, s2)
    S3-->>COORD: (R3, s3)
    S4-->>COORD: (R4, s4)
    COORD->>COORD: Aggregate R = ΣRi; compute r; s = Σsi
    COORD-->>FE: (r,s)
    FE->>C: CertificateVerify(r,s)
    FE->>C: Finished
    C->>FE: Finished
    C->>C: Verify signature using PK (standard)
```

---
## Latency Budget Example (Target)
| Component | Time (ms) |
|-----------|-----------|
| FE → COORD dispatch | 0.3 |
| COORD → Signers fan-out | 0.5 |
| Partial compute (parallel) | 1.5 |
| Aggregate & verify | 0.7 |
| COORD → FE return | 0.3 |
| Total Added (median) | ~3.3 |
(Realistic ranges depend on hardware & network; p99 should remain < 25 ms with healthy pool.)

---
## Monitoring Metrics (Handshake Focus)
- `tls_threshold_sign_duration_ms` (histogram)
- `tls_threshold_quorum_size` (gauge per request)
- `tls_threshold_precompute_pool_depth` (gauge)
- `tls_threshold_abort_count` (counter)
- `tls_threshold_malicious_flagged` (counter)

---
## Summary
The threshold ECDSA integration preserves TLS 1.3 wire semantics while substituting a distributed signing microprotocol for local private key operations. Precomputation and optimized aggregation keep latency low; audit and blame mechanisms maintain security and integrity.

---
End of workflow document.
