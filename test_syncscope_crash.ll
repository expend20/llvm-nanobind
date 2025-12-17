; Minimal test case that reproduces the syncscope crash

define void @test(ptr %ptr) {
  %a = atomicrmw volatile xchg ptr %ptr, i8 0 syncscope("agent") acq_rel, align 8
  ret void
}
