# Softimage to Arnold

_Softimage to Arnold_ (or **SItoA**) is the Arnold plugin for Autodesk Softimage. 

This repository contains the official SItoA plugin source code.
[Solid Angle](https://www.solidangle.com/), the company behind the Arnold renderer, and now part
of [Autodesk](https://www.autodesk.com/), developed the SItoA plugin commercially from 2009 to 2017.

After the Softimage end-of-life announcement in April 2014, Solid Angle 
committed to continue the development and maintenance of SItoA for at 
least a year, and then extended this period until July 2017, porting SItoA 
to the new Arnold 5 API along the way.

Solid Angle and Autodesk are now making SItoA available to the community 
under an Apache 2.0 open source license.


#### Requirements

* Softimage 2015 SP1
* Arnold 5.1.0.0 or newer
* Python 2.6 or newer
* Visual Studio 2012 (Windows)
* GCC 4.2.4 (Linux)

On Linux, newer compilers may work but require modifying the Softimage installation to
remove `libstdc++.so`.


### Building

On Windows, to build the plugins and the shaders, go to the top level directory
and run:

```
abuild
```

`abuild` is a convenience script that invokes a SItoA-local install of [​Scons](http://scons.org/), a multi-platform build system written in Python. `abuild` forwards all of its arguments to Scons. Note that you do not need to install Scons, we already distribute it in the `contrib` directory and `abuild` knows where to find it.

On Linux, prefix all commands with `./`, for example:

```
./abuild
```

For simplicity, We will omit the `./` in all following command line
examples.

#### Build Configuration

The different paths and options are handled by setting build parameters. Using
`abuild -h` will give you a complete list. You can pass them in the command line:

```
abuild MODE=opt TARGET_ARCH=x86_64
```

For commonly used values you will normally edit your custom.py file (located in the trunk directory) and set the variables there. You will have to create this file the first time. This is an example of a `custom.py` file:

```python
TARGET_ARCH         = 'x86_64'
MSVC_VERSION        = '11.0'
VS_HOME             = r'C:/Program Files (x86)/Microsoft Visual Studio 11.0/VC'
WINDOWS_KIT         = r'C:/Program Files (x86)/Windows Kits/8.0'

XSISDK_ROOT         = r'C:/Program Files/Autodesk/Softimage 2015/XSISDK'
ARNOLD_HOME         = r'C:/SolidAngle/Arnold-5.0.1.1/win64'

TARGET_WORKGROUP_PATH  = r'./Softimage_2015/Addons/SItoA'

WARN_LEVEL = 'strict'
MODE       = 'debug'
SHOW_CMDS  = True
```

Default configuration files for Windows and Linux reside in `config`. If you
want to customize the build environment variables, we suggest you copy the
config file corresponding to your platform at the top level as `custom.py` as
a starting point. 

#### Build Targets

To build a specific target, run:

```
abuild <target>
```

Running `abuild -h` will give you a list of available targets.

For example, to build the shaders only:

```
abuild shaders
```

To generate Visual Studio C++ solution and projects:

```
abuild solution
```

To build and install the plugins and the shaders in `TARGET_WORKGROUP_PATH`  
(usually your Softimage add-on location), run:

```
abuild install
```

To generate a Softimage add-on, run:

```
abuild pack
```


### Running the testsuite

A regression testsuite is included with 265 scenes and reference pictures.
It allows you to check whether changes introduce new bugs or previously fixed 
bugs (i.e. regressions). It's highly recommended to run the testsuite after any
change (bugfix, feature addition etc.) to the plugin is made. It would be also
very useful if you could add new tests:

- If you are fixing a bug, creating a regression test upfront will ensure that
you can reproduce the original problem and then verify that it has been fixed.
This will save you the effort of testing the fix manually and then creating a
separate regression test later to submit with your bugfix.

- If you are developing a new feature, designing tests to exercise the new
feature early in the development process will allow you to test your code as
you develop it, in the spirit of test-driven development.

Softimage must define a workgroup connection to the SItoA `TARGET_WORKGROUP_PATH`
folder for the testsuite to work.

To run the entire testsuite:

```
abuild testsuite
```

This will process all the scenes, and compile a HTML file with the results, 
including a diff image for the tests that failed.

To run a specific test scene -- say test_0123:

```
abuild test_0123
```

To make a new test, type:

```
abuild maketest
```

This will create a new test in the `testsuite` folder, which contains:

```
\data      (scene data, texture maps etc)
\ref       (reference image)
README     (description of the test)
```

You will find a placeholder scene file called `test.scn` in the `data` folder.
The test is checked by rendering the scene and comparing the result with the
reference image. If the difference is bigger than a predefined threshold,
the test will fail.

To re-generate the reference image for a specific test, type:

```
abuild test_0005 UPDATE_REFERENCE=true
```

Finally, you should provide a description of the test in the `README` file,
including both a one-line summary and an optional, longer, more detailed description.

You can list all the tests with their one-line description with:

```
abuild testlist
```


### Contributing

Please report issues and submit pull requests at https://github.com/Autodesk/sitoa.

Note that the old Trac server is not used anymore.

#### Language and Coding Style

Because Softimage has a C++ API and Arnold has a C API, the natural language choice for SItoA was C++. Smaller parts of SItoA are written in [JavaScript](http://en.wikipedia.org/wiki/JavaScript). We assume you have experience writing C/C++ code.

As in any non-trivial software project, it's very important that certain coding rules and guidelines are observed. Without these rules, the code added by different developers, each with a different background and personal preferences, would quickly grow without recognizable structure, and programmers would be forced to learn and identify a multitude of styles and syntactical patterns. It's much better to agree on a common set of rules so that everybody can "speak the same language". This is known as the [coding style](http://en.wikipedia.org/wiki/Programming_style). Consistency of style helps everyone in the team understand each other's code better and makes maintenance easier. Our rules for SItoA development are described below.

#### General rules and editing

 - Some advanced language features, such as complex templates and multiple inheritance, can lead to non-obvious code that is difficult for others to understand. Avoid these features, unless they produce an arguably cleaner overall design that is still easy to understand. When in doubt, opt for longer and clear code over shorter and obfuscated code. By writing code that can be clearly understood and maintained even by new, junior members in the team, you will maximize the code's business value.
 
 - Indent with three (3) real spaces: tabs are forbidden
 
 - Use the [Allman indent style](http://en.wikipedia.org/wiki/Indent_style), where each opening or closing brace has its own line:

```cpp
if (a == b)
{
   DoSomething();
   DoMoreThings(a, b);
}
else
{
   DoSomethingElse();
}
FinalThing();
```

   In cases where the block of code has a single line, it's OK to avoid braces altogether:

```cpp
for (i=0; i<count; i++)
   sum += i;

if (ret != CStatus::OK)
   return CStatus::Fail;
else
   return CStatus::OK;
```

 - There is no space between the name of a method or function and its parenthesis, neither between the parenthesis and the arguments:

```cpp
MyFunction(a, b);    // OK
MyFunction (a, b);   // wrong
MyFunction( a, b );  // wrong
```

 - End source files with an empty line

 - Header files should be included from local to global. First local files, then other headers in the same project, then headers from external libraries, and finally system headers. If any of those groups has several files, use alphabetical order.

 - Use C++ notation for include files:

```cpp
#include <cstring>   // OK
#include <string.h>  // wrong
```

#### Comments

 - Use C++ comments (`//`), not C comments (`/* ... */`). There must be a space following the `//`. For example:

```cpp
// I am a nice comment
//But I suck
```

 - Always precede classes, methods and functions with a Doxygen-style comment. For classes, describe its responsibilities and dependencies. For methods and functions, describe what the function does **and** describe each individual parameter, as well as the return value. Follow the syntax in this example:

```cpp
// Short description in one phrase (in this case: Add two numbers together).
//
// Optionally, here goes a much more detailed description of the function.
// This longer description can spawn multiple lines, include code samples, etc.
//
// @param a   This is the first floating point number to be added
// @param b   And this is the second number
// @return    The sum a + b
//
float MyAddFunction(float a, float b)
{
   ...
}
```

#### Naming conventions

 - File names use [UpperCamelCase](http://en.wikipedia.org/wiki/CamelCase). For example, the name of the file that contains the implementation of the `RenderInstance` class should be:

```cpp
RenderInstance.cpp   // OK
renderInstance.cpp   // wrong
render_instance.cpp  // wrong
Render_Instance.cpp  // wrong (mixing two different styles is specially bad)
```

   One exception is for source files that represent Arnold nodes (shaders, drivers, etc). It's common for Arnold nodes to have all lower case names: `shaders/src/material_phong.c`, `shaders/src/sib_channel_picker.c`, etc.

 - Macros are all upper case:

```cpp
#define UPDATE_UNDEFINED   0
#define UPDATE_CAMERA      1
```

 - Variable names use lowerCamelCase:

```cpp
Property arnoldOptions;    // OK
Property ArnoldOptions;    // wrong
Property arnold _options;  // wrong
```

   Do **not** use hungarian notation for variables, like `unsigned long ulCount`

 - Classes, types, functions and all other identifiers use UpperCamelCase:

```cpp
CStatus ArnoldRenderInit(void);  // OK
CStatus arnoldRenderInit();      // wrong
CStatus ArnoldRender_Init();     // wrong
cstatus ArnoldRenderInit(void);  // wrong
```

 - Member variables are prefixed with `m_`:

```cpp
CString m_renderType;
```

 - Global variables are prefixed with `g_`

```cpp
RendererContext g_renderContext;
```

 - In the header file guards (which you should always use), the macro name should exactly reflect the filename but written in upper case. For example, in the file `RenderInstance.h`:

```cpp
#ifndef RENDERINSTANCE_H
#define RENDERINSTANCE_H
...
#endif // RENDERINSTANCE_H
```

For anything else not in these rules, use common sense, but keep it consistent. If you are modifying an existing function or method, respect its original coding style. The same applies to adding code to an existing file: **respect the coding style around you**.


### Acknowledgments

Before it was open-sourced, throughout the years, SItoA has been developed by:

- Luis Armengol 
- Borja Morales 
- Stefano Jannuzzo

With contributions by:

- Andreas Bystrom
- Steven Caron
- Julien Dubuisson
- Steffen Dunner
- Michael Heberlein
- Paul Hudson
- Halfdan Ingvarsson
- Vladimir Jankijevic
- Alan Jones
- Guillaume Laforge
- Thomas Mansencal
- Helge Mathee
- Eric Mootz
- Holger Schoenberger
- Frederic Servant
- Jules Stevenson

After open-sourcing, development has been continued by:

- Jens Lindgren

Special thanks to all the users who passionately provided feedback, production
assets, bug reports and suggested features during those years.
