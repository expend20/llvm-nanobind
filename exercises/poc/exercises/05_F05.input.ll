; ModuleID = 'f05_bad_phi'
source_filename = "f05_bad_phi"

define i32 @bad(i1 %c) {
entry:
  br i1 %c, label %t, label %f
t:
  br label %m
f:
  br label %m
m:
  %x = phi i32 [ 1, %t ]
  ret i32 %x
}
