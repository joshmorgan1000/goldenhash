# Modular Rotation Theory for CROCS

## Key Insight

We don't need to be limited to powers of 2 for hash functions. By working in different number bases and using modular arithmetic, we can create hash functions for arbitrary table sizes.

## Theory

### Traditional Bit Rotation (Power of 2)
```
rotl(x, k) = (x << k) | (x >> (bits - k))
```

### Generalized Modular Rotation
For non-power-of-2 modulus N:
1. Express N in a suitable base (10, φ, or factorization)
2. "Rotate" by rearranging digits in that base
3. Use modular arithmetic to stay within bounds

### Golden Ratio Base
- Numbers can be expressed in base φ where φ = 1.618...
- Property: φ² = φ + 1
- Natural for golden ratio hashing

### Mixed-Radix Representation
For N with factorization N = p₁ × p₂ × ... × pₖ:
- Represent x as (d₁, d₂, ..., dₖ) where dᵢ < pᵢ
- Rotation becomes permutation of digits
- Preserves modular structure

## Implementation Strategy

1. Factor N to find suitable base representation
2. Use golden ratio for prime selection
3. Apply modular "rotation" via multiplication and modular reduction
4. Combine with golden ratio primes for mixing

## Special Cases

- **N is prime**: Use N-1 as working modulus or multiplicative group
- **N is highly composite**: Use factorization for mixed-radix
- **N near power of 2**: Can use hybrid bit/modular operations