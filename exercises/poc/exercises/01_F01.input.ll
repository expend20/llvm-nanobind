; ModuleID = 'f01_input'
source_filename = "f01_input"

define i32 @add1(i32 %x) {
entry:
  %sum = add i32 %x, 1
  ret i32 %sum
}
