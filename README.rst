August 9th 2008 / Ville Tuulos


Aino - a Content-Based Search Engine
====================================

This package contains source code for the Aino search engine. In brief,
Aino is a system for building optimized indices for documents made of
discrete attributes (words). Once the index has been built, documents in
the index can be ranked against queries using statistical measures based
on attribute co-occurrences.

Aino was developed in the Complex System Computation (CoSCo) research
group at the Helsinki Institute for Information Technology and
University of Helsinki, as part of the project Search-In-a-Box (SIB). Our
home page is at http://cosco.hiit.fi.

I apologize that the package doesn't contain proper documentation (yet).
However, the overall design of the system is described in my M.S. thesis,
"Design and Implementation of a Content-Based Search Engine" that can be 
found at

http://www.cs.helsinki.fi/u/tuulos/tuulos-thesis.pdf

As I'm fully responsible for the lacking documentation, you're very welcome
to contact me directly if you have any questions about the package. Reach 
me at tuulos@cs.helsinki.fi.


Contents
--------

This package contains the following subdirectories:

preproc/   Indexing pipeline. See wp/index/indexwp.sh to get an idea how 
           different programs work together. You need this stuff for 
	   building new indices from raw text documents.

index/     Query processing and ranking. index/dexvm.[c|h] is the core 
           interface for the index. You need this stuff for ranking documents
	   in the index, given a query.

ainopy/    Python interface for the index. You need this stuff if you want to
           use the indices in Python, which is currently the preferred way to
	   use them.

lib/       Utility modules.

lang/	   Snowball-library and modules related to language recognition and 
           stemming.

wp/	   Scripts for building distributed search engine for Wikipedia

aino.fi/   Scripts for building aino.hiit.fi

utils/	   Scripts used in compiling

examples/  An example script that shows how to rank documents using the Python 
           interface


Compiling
---------

Aino depends on the following packages in Debian:

scons
binutils-dev
libjudy-dev
python2.4-dev
flex
gcc
libc6-dev

It should be easy to find the corresponding packages in other distros.

Aino uses a build system called Scons (http://scons.org) instead of traditional 
Makefiles. To compile the package, say

scons
scons

(twice, really -- I'll fix it at some point..)


That's all for now. Happy hacking!



.. image:: https://d2weczhvl823v0.cloudfront.net/tuulos/aino/trend.png
   :alt: Bitdeli badge
   :target: https://bitdeli.com/free

