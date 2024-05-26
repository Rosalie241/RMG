Tue Dec 24 22:41:39 EST 1996

SCO's awk has a -e option which is similar to gawk's --source option,
allowing you to specify the script anywhere on the awk command line.

This can be a problem, since gawk will install itself as `awk' in
$(bindir). If this is ahead of /bin and /usr/bin in the search path,
several of SCO's scripts that use -e will break, since gawk does not
accept this option.

The solution is simple. After doing a `make install', do:

	rm -f /usr/local/bin/awk	# or wherever it is installed.

This removes the `awk' symlink so that SCO's programs will continue
to work.

If you complain to me about this, I will fuss at you for not having
done your homework.

Arnold Robbins
arnold@skeeve.com

---------------------------
Date: 14 Oct 1997 12:17 +0000
From: Leigh Hebblethwaite <LHebblethwaite@transoft.com>
To: bug-gnu-utils@prep.ai.mit.edu

I've just built gawk 3.0.3 on my system and have experienced a problem 
with the routine pipeio2.awk in the test suite. However the problem 
appears to be in the tr command rather than gawk.

I'm using SCO Open Server 5. On the version I have there appears to be 
a problem with tr such that:

	tr [0-9]. ...........

does NOT translate 9s. This means that the output from:

	echo " 5  6  7  8  9 10 11" | tr [0-9]. ...........

is:

	 .  .  .  .  9 .. ..

This problem causes the pipeio2 test to be reported as a failure.

Note that the following variation on the tr command works fine:

	tr 0123456789. ...........

For your info the details of my system are summarised by the out put 
of the uname -X command, which is:

System = SCO_SV
Node = sgscos5
Release = 3.2v5.0.2
KernelID = 96/01/23
Machine = Pentium
BusType = EISA
Serial = 4EC023443
Users = 5-user
OEM# = 0
Origin# = 1
NumCPU = 1


