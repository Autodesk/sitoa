# SItoA
The Arnold plugin for Softimage, version 4.1

Building and Installation
-------------------------

**Requirements:**

* Python 2.6 or newer
* The Arnold SDK
* The Softimage SDK
* Microsoft Visual Studio 2012 (Windows)
* gcc-4.2.4 (Linux)

Newer compilers may work too.

Build relies on **Scons**, distributed in the **contrib** directory.
The build environment variables for Scons must be set in a file called
**custom.py**. A template file is included, that you should modify based on 
your local setup.

To build the plugins and the shaders, run

**abuild**

To build and install the plugins and the shaders in a given directory 
(usually your Softimage addon location), run 

**abuild install**

A testsuite is also included with 265 scenes, each with its reference picture.
To run the testsuite, run

**abuild testsuite**

This will process all the scenes, and compile a html file with the results, 
including a diff image for the tests that failed.

To test just one specific scene (say test_0123), run

**abuild test_0123**

To generate the full Softimage addon, run

**abuild pack**
