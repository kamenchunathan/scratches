++++++                         cell 0 number for which to calc fibo
                              
[>>>>>+>+<<<<<<-]              copy cell 0 into cells 5 and 6
>>>>>>[<<<<<<+>>>>>>-]         restore cell 0 from cell 6
<<<<<<                         move ptr back to cell 0
                              
-                              correction for starting conditions 0 1
                              
>                              cell 1 1st fibo 0 (later n minus 2)
>+                             cell 2 2nd fibo 1 (later n minus 1)
<<                             put ptr back on cell 0
[                              start loop for wanted fibo number
                              
                               everything in this loop is about adding
                               (n minus 2) and (n minus 1)
                              
>[>>+>+<<<-]                   copy cell 1 into cells 3 and 4
>>>[<<<+>>>-]                  restore cell 1 from cell 4
                              
                               cell 3 now holds (n minus 2)
                              
<<[>+>+<<-]                    add cell 2 to cell 3 and copy it to cell 4
>>[<<+>>-]                     restore cell 2 from cell 4
                              
                               cell 3 now holds (n minus 2) plus (n minus 1)
                              
<<<[>>>+<<<-]                  clear cell 1 by moving it to cell 4
>[<+>-]                        move cell 2 to cell 1 clearing cell 2
>[<+>-]                        move cell 3 to cell 2 clearing cell 3
                              
<<<-]                          decrement cell 0 and loop again if not null
                              
                               end of calculation; output the result:
                              
>>>>>>>                        cell 7 is used to load cell 8 with letters
++++++++++[>++++++++<-]>++++   to start set cell 8 to 84 (T)
>++++[>++++++++<-]             cell 9 is used to load cell 10 with a blank
<                              start printing
.                              T
++++++++++ ++++++++++.         h
---.                           e
>>.<<                          print blank
+.                             f
+++.                           i
-------.                       b
+++++++++++++.                 o
-.                             n
-------------.                 a
++..                           cc
++++++.                        i
>>.<<                          print blank
+++++.                         n
+++++++.                       u
--------.                      m
-----------.                   b
+++.                           e
+++++++++++++.                 r
>>.<<                          print blank
------------.                  f
+++++++++.                     o
+++.                           r
>>.<<                          print blank
<<<                            move ptr to cell with seed number (5)
++++++++++ ++++++++++          add 48 to get right number char (we
++++++++++ ++++++++++          can add the number is no longer needed)
++++++++ .                     print seednumber as char
>>>>>.<<                       print blank
---------.                     i
++++++++++.                    s
>>.<<                          print blank
<<<<<<                         move ptr to cell with result (2)
++++++++++ ++++++++++          add 48 to get right number char
++++++++++ ++++++++++          no idea so far how to handle
++++++++ .                     numbers > 9
>>>>>>>                        back to cell with blank
---------- ---------- --.      dec to 10 to get newline char

read number
>,[>++++++[-<-------->]>+++++++++[-<<<[->+>+<<]>>[-<<+>>]>]<<[-<+>],]

print first numbers (1)
<<+++++.-----.+++++.-----

initialize sequence
>-->+>+

start loop
<<
[-
  move / copy second
  >>[->+>+<<]
  sum first / second
  <[->>>+<<<]
  move second to first place
  >>[-<<+>>]
  move / copy sum
  >[-<<+>>>+<]
  print comma
  <<<<<.
  print number
  >>>>>>[>>>>++++++++++<<<<[->+>>+>-[<-]<[->>+<<<<[->>>+<<<]>]<<]>+[-<+>]>>>[-]>[-<<<<+>>>>]<<<<]<[>++++++[<++++++++>-]<-.[-]<]<<<<
]
