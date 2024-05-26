Sat Jan 28 22:07:17 EST 1995

Some older versions of yacc (notably Ultrix's) have limits on the depth
of the parse stack. This only shows up when gawk is dealing with deeply
nested control structures, such as those in `awf'.

The problem goes away if you use either bison or Berkeley yacc.

Arnold Robbins
arnold@skeeve.com
