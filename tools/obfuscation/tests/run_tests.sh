#!/bin/bash
# Test script for obfuscation tools
# Runs each tool with fixed seed and produces deterministic output

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../../../build"
LLVM_PREFIX=$(cat "${SCRIPT_DIR}/../../../.llvm-prefix" 2>/dev/null || echo "/usr/local")
LLVM_AS="${LLVM_PREFIX}/bin/llvm-as"
LLVM_DIS="${LLVM_PREFIX}/bin/llvm-dis"

# Function to normalize output (remove absolute paths)
normalize() {
    sed -e "s|${SCRIPT_DIR}/||g" -e 's|/[^ ]*obfuscation/tests/||g'
}

# Create test bitcode
"${LLVM_AS}" "${SCRIPT_DIR}/simple.ll" -o "${SCRIPT_DIR}/simple.bc"
"${LLVM_AS}" "${SCRIPT_DIR}/large_blocks.ll" -o "${SCRIPT_DIR}/large_blocks.bc"
"${LLVM_AS}" "${SCRIPT_DIR}/strings.ll" -o "${SCRIPT_DIR}/strings.bc"

echo "=== MBA Substitution Test ==="
"${BUILD_DIR}/mba_sub" --seed=42 "${SCRIPT_DIR}/simple.bc" "${SCRIPT_DIR}/mba_out.bc"
"${LLVM_DIS}" "${SCRIPT_DIR}/mba_out.bc" -o - | normalize

echo ""
echo "=== Basic Block Splitter Test ==="
"${BUILD_DIR}/bb_split" --seed=42 --min-size=5 --max-size=8 --chance=100 \
    "${SCRIPT_DIR}/large_blocks.bc" "${SCRIPT_DIR}/bb_out.bc"
"${LLVM_DIS}" "${SCRIPT_DIR}/bb_out.bc" -o - | normalize

echo ""
echo "=== Indirect Branch Test ==="
"${BUILD_DIR}/indirect_branch" --seed=42 --chance=100 \
    "${SCRIPT_DIR}/simple.bc" "${SCRIPT_DIR}/ibr_out.bc"
"${LLVM_DIS}" "${SCRIPT_DIR}/ibr_out.bc" -o - | normalize

echo ""
echo "=== Control Flow Flattening Test ==="
"${BUILD_DIR}/control_flow_flatten" --seed=42 \
    "${SCRIPT_DIR}/simple.bc" "${SCRIPT_DIR}/cff_out.bc"
"${LLVM_DIS}" "${SCRIPT_DIR}/cff_out.bc" -o - | normalize

echo ""
echo "=== Control Flow Flattening with SipHash Test ==="
"${BUILD_DIR}/control_flow_flatten" --seed=42 --use-siphash=100 \
    "${SCRIPT_DIR}/simple.bc" "${SCRIPT_DIR}/cff_siphash_out.bc"
"${LLVM_DIS}" "${SCRIPT_DIR}/cff_siphash_out.bc" -o - | normalize

echo ""
echo "=== String Encryption Test ==="
"${BUILD_DIR}/string_encrypt" --seed=42 --mode=global \
    "${SCRIPT_DIR}/strings.bc" "${SCRIPT_DIR}/str_out.bc" 2>&1 | normalize
"${LLVM_DIS}" "${SCRIPT_DIR}/str_out.bc" -o - | normalize

echo ""
echo "=== All tests passed ==="
