; Simple test module for obfuscation tools

define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}

define i32 @sub(i32 %a, i32 %b) {
entry:
  %diff = sub i32 %a, %b
  ret i32 %diff
}

define i32 @mul(i32 %a, i32 %b) {
entry:
  %prod = mul i32 %a, %b
  ret i32 %prod
}

define i32 @xor_func(i32 %a, i32 %b) {
entry:
  %result = xor i32 %a, %b
  ret i32 %result
}

define i32 @or_func(i32 %a, i32 %b) {
entry:
  %result = or i32 %a, %b
  ret i32 %result
}

define i32 @factorial(i32 %n) {
entry:
  %cmp = icmp sle i32 %n, 1
  br i1 %cmp, label %base, label %recurse

base:
  ret i32 1

recurse:
  %n_minus_1 = sub i32 %n, 1
  %fact_n_minus_1 = call i32 @factorial(i32 %n_minus_1)
  %result = mul i32 %n, %fact_n_minus_1
  ret i32 %result
}

define i32 @max(i32 %a, i32 %b) {
entry:
  %cmp = icmp sgt i32 %a, %b
  br i1 %cmp, label %then, label %else

then:
  br label %end

else:
  br label %end

end:
  %result = phi i32 [ %a, %then ], [ %b, %else ]
  ret i32 %result
}
