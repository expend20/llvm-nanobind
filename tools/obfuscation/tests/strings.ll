; Test module with strings for string_encrypt testing

@.str.hello = private unnamed_addr constant [14 x i8] c"Hello, World!\00"
@.str.secret = private unnamed_addr constant [17 x i8] c"Secret password!\00"
@.str.test = private unnamed_addr constant [12 x i8] c"Test string\00"

declare i32 @puts(ptr)

define i32 @main() {
entry:
  %0 = call i32 @puts(ptr @.str.hello)
  %1 = call i32 @puts(ptr @.str.secret)
  %2 = call i32 @puts(ptr @.str.test)
  ret i32 0
}
