# Things to Investigate

## Questions:

- Can the same principle be applied to one-way hash functions?
- Could combining multiple collision-optimized hash domains (CROCS-optimized N-count hashes) be leveraged to achieve cryptographic hardness?
- Is there an asymmetrical, “prime-like” security hidden within combining multiple optimized hash spaces?
- Can we create cryptographic structures from deterministic hash collisions in multiple, carefully chosen N-sized hash spaces?

## Possible Next Steps

### Hybrid Hash-Based Commitment Schemes:

Consider using multiple CROCS-optimized hashes to create statistically strong cryptographic commitments.

### Key Derivation Functions (KDF):

Create multi-layered KDFs with CROCS-optimized collision domains, ensuring statistical hardness against brute-force attempts.

### Zero-Knowledge Proof (ZKP) protocols:

Carefully optimized collision spaces might provide statistical soundness and completeness properties essential in ZKP.

## To Do Regardless of Next Steps:

*(I have a math professor friend who may be able to help with this.)*

- Formalize the mathematical model of CROCS.
- Research existing literature and cryptographic primitives that could be adapted to CROCS.
- More experiments, more data collection
