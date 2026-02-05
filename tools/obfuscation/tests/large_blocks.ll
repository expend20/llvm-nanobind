; Test module with larger basic blocks for testing bb_split

define i32 @compute(i32 %a, i32 %b, i32 %c) {
entry:
  %t1 = add i32 %a, %b
  %t2 = mul i32 %t1, %c
  %t3 = sub i32 %t2, %a
  %t4 = xor i32 %t3, %b
  %t5 = or i32 %t4, %c
  %t6 = and i32 %t5, %a
  %t7 = shl i32 %t6, 2
  %t8 = lshr i32 %t7, 1
  %t9 = add i32 %t8, %t1
  %t10 = mul i32 %t9, %t2
  %t11 = sub i32 %t10, %t3
  %t12 = xor i32 %t11, %t4
  %t13 = or i32 %t12, %t5
  %t14 = and i32 %t13, %t6
  %t15 = add i32 %t14, %t7
  ret i32 %t15
}
