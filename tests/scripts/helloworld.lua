local Test = {}
function Test:test() print("hello world, from a script file !") end
Test:test()
print("... and from a direct command")
local a = 21
print("And the answer is: ".. a*2 .." !")
