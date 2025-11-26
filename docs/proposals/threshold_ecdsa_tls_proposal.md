# Proposal: Integrating Fast Multiparty Threshold ECDSA with Fast Trustless Setup into TLS

## 1. Executive Summary
This proposal describes how to integrate a modern Fast Multiparty Threshold ECDSA scheme (e.g., Lindell–Nof style two-round signing with aggressive precomputation) coupled with a Fast Trustless Distributed Key Generation (DKG) into the TLS 1.3 ecosystem. The goal is to eliminate single points of compromise for private keys used in server authentication (and optionally CA issuance), improve resilience, and enable cryptographic agility while keeping wire compatibility with existing TLS clients.

## 2. Objectives
- Replace single-server ECDSA private keys with threshold-protected keys whose shares are held by N signer nodes; any subset of size T forms a quorum for signatures.
- Achieve a trustless (no dealer) key setup: no single party ever sees the full key; robustness against up to f faulty nodes (Byzantine or malicious) during DKG.
- Maintain TLS 1.3 wire compatibility: no client changes required for server authentication.
- Minimize added handshake latency; target < 10 ms added median signing delay for Internet-scale deployments.
- Support scalable precomputation to batch nonce generation and inversion needed for ECDSA threshold signing.
- Provide secure rotation, refresh, and revocation workflows.

## 3. Scope
IN-SCOPE:
- Server certificate private key (ECDSA P-256 / secp256r1 or secp256k1) thresholdization.
- Optional threshold CA for internal PKI issuance.
- TLS 1.3 handshake integration (CertificateVerify, OCSP response signing, SCT/log signatures).
- Operational tooling, monitoring, audit logging.

OUT-OF-SCOPE (initial phase):
- Client certificate threshold signing.
- Post-quantum threshold schemes (future extension).
- Cross-organizational federation of shares.

## 4. Non-Goals
- Modifying TLS protocol messages or requiring client-side library changes.
- Introducing proprietary extensions that break standard compliance.

## 5. Stakeholders
- Security Engineering (protocol design & threat modeling).
- Platform/Infrastructure (deployment, scaling, observability).
- Compliance/Governance (key custody attestations, audits).
- DevOps / SRE (uptime & failure recovery).
- Application Teams (consume certificates transparently).

## 6. Background & Motivation
Single private keys are high-value targets. HSMs reduce risk but remain single logical roots. Threshold ECDSA disperses trust: compromise of < T nodes yields no key reconstruction. Fast trustless DKG eliminates dealer risk. Precomputation techniques (batch nonce & partial inversions) allow near-HSM latency. Integrating with TLS maintains ecosystem compatibility while raising the bar against key exfiltration and signing misuse.

## 7. Requirements
### Functional
- Generate distributed ECDSA key via DKG with N ≥ 5 and threshold T (e.g., T = 3) configurable.
- Produce valid RFC 8446-compliant ECDSA signatures for CertificateVerify.
- Support proactive share refresh without changing the public key (preventing long-term leakage accumulation).
- Provide API: `Sign(data)` returning standard ECDSA (r,s) pair; deterministic binding to handshake transcript.
- High availability: gracefully handle up to f offline/malicious nodes (< T impact).

### Security
- Unforgeability assuming threshold hardness + standard ECDSA assumptions.
- Confidentiality of shares at rest (encryption + hardware isolation optional).
- Auditability: tamper-evident logs of signing requests (hash of transcript, timestamp, quorum set).
- Side-channel resistance: constant-time operations, avoidance of nonce bias.
- Robustness: detection and exclusion of misbehaving share holders.

### Performance
- Median added latency ≤ 10 ms; p99 ≤ 25 ms under nominal load.
- Sustain ≥ X signatures/sec (define target, e.g., 500) across cluster via precomputation pool.
- Horizontal scaling: add nodes to increase redundancy and precomputation throughput.

### Operational
- Automated health checks of signer nodes (liveness + cryptographic correctness proofs for precomputed tuples).
- Secure key rotation (new DKG) with zero downtime using rolling dual-key period.
- Disaster recovery plan (threshold backups of encrypted shares with multi-role access controls).

## 8. Threat Model & Security Goals
Adversaries:
- External attacker compromising subset of nodes (< T).
- Insider with access to logs and some shares.
- Network attacker intercepting signing traffic.
- Malicious node providing incorrect partial signatures/nonces.

Security Goals:
- No partial leakage enables forging (robustness vs nonce reuse).
- Mitigate rogue-key attack during DKG (verification of commitment & coefficient sharing).
- Detect and evict nodes causing aborts or invalid shares.
- Provide strong audit trail to attribute misuse attempts.

## 9. Proposed Architecture
### Components
- Signer Nodes (S1..SN): Hold private shares, run DKG, maintain precompute pool.
- Coordinator (stateless or minimal state): Orchestrates signing rounds; can be replicated or leader-elected (Raft/etcd) but not a trust anchor.
- Auditor Service: Consumes logs, runs consistency checks, detects anomalies (e.g., excessive aborts).
- Precomputation Workers: May be co-located with signers or separate microservice generating nonce commitments.
- PKI Integration Layer: Interfaces with existing CA (standard or threshold CA cluster).
- Monitoring/Telemetry Stack: Prometheus/OpenTelemetry metrics (latency, quorum health, abort rates).

### Data Flows
1. DKG Phase: Each signer broadcasts polynomial commitments & shares over authenticated channels (TLS mutual auth using bootstrap temporary keys), verifies, computes local share.
2. Precomputation: Signers create tuples (k_i, R_i, k_i^{-1} * share adjustments) aggregated later.
3. Signing: Coordinator collects active partials from ≥ T signers; computes aggregated R and final s using threshold formula; returns (r,s) to TLS frontend.
4. TLS Frontend: OpenSSL provider or PKCS#11 engine requests signature; receives (r,s); passes to handshake as if from local key.

## 10. Cryptographic Protocol Details (High-Level)
### Distributed Key Generation (Fast Trustless)
- Curve: secp256r1 (widely accepted for TLS). Optionally secp256k1 if ecosystem supports.
- Use a robust DKG (e.g., Gennaro & Jarecki style improvements) with commitments via Pedersen or Feldman + zero-knowledge proofs of correct coefficient formation.
- Each party selects polynomial f_i of degree (T-1); broadcasts commitments C_{i,j}.
- Share for party p: s_p = Σ f_i(p). Public key: Q = Σ G * f_i(0). All verify Q integrity with commitments.
- Abort if any inconsistency; restart with exclusion list.

### Precomputation
- Create pool of nonces: Each signer chooses random k_i, computes commitment R_i = k_i*G, plus needed auxiliary values for inversion and later combination.
- Coordinator forms aggregated R = Σ R_i; r = x(R) mod n; Broadcast r to signers for partial s_i computations.

### Signing (Fast Threshold ECDSA)
- TLS transcript hash m prepared by frontend and distributed.
- Each signer uses its share x_i and precomputed k_i to produce partial: s_i = k_i^{-1} * (m + r * x_i) mod n (with appropriate tweaks depending on scheme variant).
- Coordinator aggregates s = Σ s_i mod n.
- Output signature (r,s) after malleability adjustment (enforce low-s per RFC 6979 style stamping or BIP-62-like rule).

### Share Refresh
- Periodic proactive refresh: run secondary polynomial exchange to randomize shares without changing public key Q.

### Misbehavior Detection
- Invalid s_i or inconsistent R_i triggers blame protocol: require signer to reveal commitments/proofs; proven malicious nodes quarantined.

## 11. TLS Integration Points
1. Certificate Private Key: Store only public cert locally; private key replaced by provider hooking ECDSA_sign → threshold RPC.
2. OpenSSL 3 Provider / Engine: Implement `EVP_PKEY_sign` redirection. Maintain fallback if quorum unavailable.
3. Handshake: Unchanged messages. Signature generated just-in-time for CertificateVerify.
4. Session Tickets: No changes (uses symmetric keys); optional threshold for ticket key rotation (future).
5. OCSP / CRL / SCT: Extend signing service to handle these additional messages using same key.
6. Client Auth (Optional Future): Mirror server design for client certs in high-security clients.

## 12. Operational Workflow
1. Bootstrap: Deploy signer nodes with ephemeral bootstrap keys; run initial DKG; publish certificate with new public key.
2. Normal Signing: TLS frontend requests signature; quorum responds; signature delivered.
3. Precompute Refill: Maintain pool size (e.g., 1000) with background generation; scale horizontally.
4. Rotation: Run parallel DKG for new key; dual-publish certificate; after propagation window, deprecate old quorum.
5. Incident Response: If node compromise suspected, run refresh or partial key rotation excluding node.
6. Decommission: Securely erase share, remove from membership list.

## 13. High Availability & Failover
- Quorum design: N=7, T=4 example. Any 4 available signers suffice.
- Load balancing: Coordinator selects fastest subset (latency-aware scheduling).
- Fallback: If quorum unavailable, controlled policy (serve 5xx vs optionally hold traffic). Avoid caching old signatures.

## 14. Performance Considerations
- Parallel precomputation amortizes expensive inversions.
- Batch pipeline: For multiple concurrent handshakes, coordinate reuse of aggregated R sets.
- Latency Budget Example: Network round trip (2–3 ms intra-DC) + aggregation (1 ms) + partial computation (<2 ms) ⇒ ~5–8 ms median.
- Metrics: track pool depth, precompute failure rate, s aggregation time, aborted sign attempts.

## 15. API & Interface Contract (Draft)
`POST /v1/sign` JSON: `{ "hash": <hex>, "context": "TLS-1.3-CertificateVerify", "nonce_policy": "fresh" }`
Response: `{ "r": <hex>, "s": <hex>, "pubkey": <hex>, "quorum": [ids], "precompute_id": <uuid>, "timestamp": <iso8601> }`
Error Modes:
- 409 QuorumInsufficient
- 424 PrecomputeDepleted (auto retry)
- 400 InvalidHashFormat
- 500 InternalAggregationError

## 16. Logging & Audit
- Immutable append-only log (e.g., Kafka + hashing chain) storing: request id, transcript hash, selected signers, aggregated R, final (r,s).
- Daily Merkle root published for external audit transparency.

## 17. Security Analysis Summary
Attacks & Mitigations:
- Share Theft (< T): No key reconstruction (threshold property).
- Nonce Bias: Deterministic generation + verifiable commitments pre-signing.
- Rogue-Key During DKG: Commitment & proof verification, abort on mismatch.
- Faulty Signer DoS: Adaptive quorum selection; eviction list after threshold of faults.
- Coordinator Compromise: Cannot forge; lacks shares; at worst orchestrates slower signing — monitored via latency anomaly detection.

## 18. Testing & Validation Plan
- Unit: Arithmetic correctness, invariants (low-s normalization, R consistency).
- Integration: Local cluster (N=5, T=3) handshake with OpenSSL test client.
- Fuzzing: Malformed partial signatures, corrupted commitments.
- Chaos: Random signer termination mid-protocol; ensure quorum failover.
- Performance Benchmarks: Throughput vs number of precompute workers.
- Security: Static analysis, secret scanning, side-channel timing tests.
- Compliance: Map controls to standards (e.g., PCI DSS, SOC 2 key management) where relevant.

## 19. Implementation Phases & Timeline (Indicative)
| Phase | Duration | Deliverables |
|-------|----------|-------------|
| 0 Research | 2 wks | Formal protocol selection, curve choice finalized |
| 1 Prototype | 4 wks | Minimal signer cluster, REST signing API, unit tests |
| 2 TLS Integration | 3 wks | OpenSSL provider, handshake demo, perf baseline |
| 3 Hardening | 4 wks | Audit logging, share refresh, fault tolerance |
| 4 Rotation & CA | 3 wks | Key rotation flow, optional threshold CA prototype |
| 5 Production Rollout | 4 wks | Monitoring, docs, runbooks, DR plan |

## 20. Risk & Mitigation
| Risk | Impact | Mitigation |
|------|--------|-----------|
| Protocol Complexity Bugs | Forged/invalid signatures | Use vetted libraries, formal verification of critical arithmetic |
| Latency Spikes | Handshake slowdowns | Precompute pools, autoscale signers, latency SLO alerts |
| Coordinator Bottleneck | Throughput cap | Stateless multi-coordinator + idempotent signing sessions |
| Share Leakage Over Time | Aggregate risk | Proactive refresh schedule (e.g., weekly) + memory hardening |
| Node Malicious Behavior | Abort storms | Blame protocol + automated eviction + replacement provisioning |
| Operational Misconfig | Downtime | IaC with validated configs, canary tests, policy enforcement |

## 21. Future Extensions
- Client Certificate Threshold Signing for mutual TLS high-assurance clients.
- Post-Quantum Threshold (e.g., threshold Dilithium) after PQC stabilization.
- Multi-Cloud Share Distribution for geographic compromise resistance.
- Transparency Log Integration (e.g., RFC 6962 style) for all signatures.
- Confidential Computing (TEE) enclaves for share storage & execution.

## 22. Tooling & Dependencies (Initial Recommendations)
- Language: Rust or Go for signer nodes (strong crypto ecosystems).
- Crypto Libraries: Rust `zkcrypto`, `curve25519-dalek` (if Ed variant) / `k256` for secp256k1 / `p256` for secp256r1; or Go `btcec`/`crypto/elliptic` + custom threshold layer.
- Message Bus: gRPC or QUIC with mutual TLS for intra-cluster communication.
- Storage: Encrypted local KV (e.g., Hashicorp Vault integration for share sealing) + etcd for membership metadata.
- Observability: Prometheus + Grafana, OpenTelemetry traces for signing paths.

## 23. Compliance & Governance
- Document key ceremonies (virtual DKG transcript archive).
- Access controls: RBAC for node maintenance actions; break-glass audited.
- Regular external audits of code + configuration.

## 24. Deployment Model
- Kubernetes StatefulSet for signer nodes with anti-affinity (different AZs).
- Coordinator as Deployment with HPA.
- Secrets via sealed secrets or Vault injection (runtime decryption only).

## 25. Open Questions
- Exact threshold parameters for desired risk model (N,T)?
- Choice between secp256r1 vs secp256k1 (ecosystem preference vs performance)?
- Use of existing threshold ECDSA library vs custom implementation.
- Whether to integrate threshold CA in initial release or defer.
- Audit log external publication cadence.

## 26. Acceptance Criteria
- Demo TLS 1.3 handshake to unmodified OpenSSL client with threshold-generated signature meeting latency SLO.
- Successful share refresh without public key change.
- Simulated compromise of ≤ T-1 nodes not yielding private key reconstruction (validated via attempted attack scripts).
- Rotation achieving zero downtime across 24h observation window.

## 27. Summary
This integration elevates key security for TLS by decentralizing control and leveraging efficient modern threshold ECDSA protocols with a trustless setup. It maintains client transparency, scales horizontally, and lays a foundation for future cryptographic agility (PQC, expanded threshold usages). Proceeding with a structured phased approach mitigates complexity and ensures robust, auditable operations.

## 28. References
- RFC 8446: The Transport Layer Security (TLS) Protocol Version 1.3.
- Lindell, Nof (2021): Fast Secure Two-Party ECDSA Signing.
- Gennaro, Goldfeder (2018): Fast Multiparty Threshold ECDSA with Paillier Encryption.
- Gennaro et al. (2020): Threshold ECDSA for Cryptocurrencies.
- SEC 1: Elliptic Curve Cryptography.
- OpenSSL Provider API Documentation.
- FROST (Draft): Flexible Round-Optimized Schnorr Threshold Signatures (for future EdDSA adaptation).

---
End of proposal.
